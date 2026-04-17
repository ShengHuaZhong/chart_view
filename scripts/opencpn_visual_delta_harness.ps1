param(
  [Parameter(Mandatory = $true)]
  [string]$ManifestDir,

  [Parameter(Mandatory = $true)]
  [string]$ObservationDir,

  [Parameter(Mandatory = $true)]
  [string]$OutDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-JsonFile {
  param([string]$Path)

  if (-not (Test-Path -LiteralPath $Path)) {
    throw "Missing JSON file: $Path"
  }

  return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
}

function Get-OptionalProperty {
  param($Object, [string]$Name)

  if ($null -eq $Object) {
    return $null
  }

  if ($Object -is [System.Collections.IDictionary]) {
    if ($Object.Contains($Name)) {
      return $Object[$Name]
    }

    return $null
  }

  $property = $Object.PSObject.Properties[$Name]
  if ($null -eq $property) {
    return $null
  }

  return $property.Value
}

function Get-StringArray {
  param($Value)

  if ($null -eq $Value) {
    return @()
  }

  return @($Value | ForEach-Object { [string]$_ })
}

function New-CountMap {
  param([string[]]$Values)

  $map = [ordered]@{}
  foreach ($value in $Values) {
    if ([string]::IsNullOrWhiteSpace($value)) {
      continue
    }

    if ($map.Contains($value)) {
      $map[$value] = [int]$map[$value] + 1
    } else {
      $map[$value] = 1
    }
  }

  return $map
}

function Convert-CountMapToArray {
  param($Map)

  $rows = @()
  foreach ($key in $Map.Keys) {
    $rows += [ordered]@{
      key   = [string]$key
      count = [int]$Map[$key]
    }
  }

  return ,@($rows)
}

function Compare-FeatureField {
  param(
    [string]$FieldName,
    $Expected,
    $Observed,
    [System.Collections.Generic.List[string]]$Mismatches
  )

  if ($null -eq $Expected) {
    return
  }

  if ([string]$Expected -ne [string]$Observed) {
    $Mismatches.Add(
      "$FieldName mismatch: expected='$Expected' observed='$Observed'")
  }
}

function Compare-StringArrayField {
  param(
    [string]$FieldName,
    $Expected,
    $Observed,
    [System.Collections.Generic.List[string]]$Mismatches
  )

  if ($null -eq $Expected) {
    return
  }

  $expectedValues = @(Get-StringArray -Value $Expected)
  $observedValues = @(Get-StringArray -Value $Observed)
  if (($expectedValues.Count -ne $observedValues.Count) -or
      (Compare-Object -ReferenceObject $expectedValues -DifferenceObject $observedValues -SyncWindow 0)) {
    $Mismatches.Add(
      "$FieldName mismatch: expected='$(($expectedValues -join ', '))' observed='$(($observedValues -join ', '))'")
  }
}

function Write-JsonFile {
  param([string]$Path, $Value)

  $directory = Split-Path -Parent $Path
  if (-not (Test-Path -LiteralPath $directory)) {
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
  }

  $json = $Value | ConvertTo-Json -Depth 10
  [System.IO.File]::WriteAllText($Path, $json, [System.Text.UTF8Encoding]::new($false))
}

$manifestFiles = @(Get-ChildItem -LiteralPath $ManifestDir -Filter '*.opencpn.json' | Sort-Object Name)
if ($manifestFiles.Count -eq 0) {
  throw "No OpenCPN manifest files found under $ManifestDir"
}

$sceneSummaries = @()
$totalComparableFeatures = 0
$totalFeatureDeltas = 0
$totalCropDeltas = 0

foreach ($manifestFile in $manifestFiles) {
  $manifest = Read-JsonFile -Path $manifestFile.FullName
  $sceneId = [string](Get-OptionalProperty -Object $manifest -Name 'sceneId')
  if ([string]::IsNullOrWhiteSpace($sceneId)) {
    throw "Manifest $($manifestFile.Name) is missing sceneId"
  }

  $observationPath = Join-Path $ObservationDir "$sceneId.reference.json"
  $observation = Read-JsonFile -Path $observationPath
  if ([string]$observation.sceneId -ne $sceneId) {
    throw "Scene mismatch for ${sceneId}: observation has '$($observation.sceneId)'"
  }

  $observedFeatures = @{}
  foreach ($feature in @($observation.features)) {
    $featureId = [string]$feature.featureId
    if ($observedFeatures.ContainsKey($featureId)) {
      throw "Duplicate observed feature id '$featureId' in $observationPath"
    }

    $observedFeatures[$featureId] = $feature
  }

  $observedCrops = @{}
  foreach ($crop in @($observation.crops)) {
    $cropName = [string]$crop.name
    if ($observedCrops.ContainsKey($cropName)) {
      throw "Duplicate observed crop '$cropName' in $observationPath"
    }

    $observedCrops[$cropName] = $crop
  }

  $featureReports = @()
  $featureDeltaCount = 0
  $comparableFeatureCount = 0
  foreach ($expectedFeature in @($manifest.features)) {
    $featureId = [string]$expectedFeature.featureId
    $comparison = [string](Get-OptionalProperty -Object $expectedFeature -Name 'comparison')
    if ([string]::IsNullOrWhiteSpace($comparison)) {
      $comparison = 'strict'
    }

    $observedFeature = Get-OptionalProperty -Object $observedFeatures -Name $featureId
    $mismatches = New-Object System.Collections.Generic.List[string]
    $status = 'match'

    if ($null -eq $observedFeature) {
      $status = 'missing'
      $mismatches.Add("feature '$featureId' missing from observation")
      if ($comparison -ne 'informational') {
        ++$featureDeltaCount
      }
    } else {
      Compare-FeatureField -FieldName 'objectAcronym' -Expected $expectedFeature.objectAcronym -Observed $observedFeature.objectAcronym -Mismatches $mismatches
      Compare-FeatureField -FieldName 'sourceRcid' -Expected (Get-OptionalProperty -Object $expectedFeature -Name 'sourceRcid') -Observed $observedFeature.sourceRcid -Mismatches $mismatches
      Compare-FeatureField -FieldName 'tableName' -Expected (Get-OptionalProperty -Object $expectedFeature -Name 'tableName') -Observed $observedFeature.tableName -Mismatches $mismatches
      Compare-FeatureField -FieldName 'primaryAssetId' -Expected (Get-OptionalProperty -Object $expectedFeature -Name 'primaryAssetId') -Observed $observedFeature.primaryAssetId -Mismatches $mismatches
      Compare-FeatureField -FieldName 'textAttributeKey' -Expected (Get-OptionalProperty -Object $expectedFeature -Name 'textAttributeKey') -Observed $observedFeature.textAttributeKey -Mismatches $mismatches
      Compare-FeatureField -FieldName 'suppressed' -Expected (Get-OptionalProperty -Object $expectedFeature -Name 'suppressed') -Observed $observedFeature.suppressed -Mismatches $mismatches
      Compare-StringArrayField -FieldName 'conditionIds' -Expected (Get-OptionalProperty -Object $expectedFeature -Name 'conditionIds') -Observed $observedFeature.conditionIds -Mismatches $mismatches

      if ($comparison -eq 'informational') {
        $status = 'informational'
      } elseif ($mismatches.Count -gt 0) {
        $status = 'delta'
        ++$featureDeltaCount
      }
    }

    if ($comparison -ne 'informational') {
      ++$comparableFeatureCount
    }

    $featureReports += [ordered]@{
      featureId = $featureId
      comparison = $comparison
      status = $status
      note = [string](Get-OptionalProperty -Object $expectedFeature -Name 'note')
      expected = [ordered]@{
        objectAcronym = [string](Get-OptionalProperty -Object $expectedFeature -Name 'objectAcronym')
        sourceRcid = [string](Get-OptionalProperty -Object $expectedFeature -Name 'sourceRcid')
        tableName = [string](Get-OptionalProperty -Object $expectedFeature -Name 'tableName')
        primaryAssetId = [string](Get-OptionalProperty -Object $expectedFeature -Name 'primaryAssetId')
        textAttributeKey = [string](Get-OptionalProperty -Object $expectedFeature -Name 'textAttributeKey')
        conditionIds = Get-StringArray -Value (Get-OptionalProperty -Object $expectedFeature -Name 'conditionIds')
        suppressed = Get-OptionalProperty -Object $expectedFeature -Name 'suppressed'
      }
      observed = if ($null -eq $observedFeature) {
        $null
      } else {
        [ordered]@{
          objectAcronym = [string]$observedFeature.objectAcronym
          ruleId = [string]$observedFeature.ruleId
          sourceRcid = [string]$observedFeature.sourceRcid
          tableName = [string]$observedFeature.tableName
          styleKey = [string]$observedFeature.styleKey
          primaryAssetId = [string]$observedFeature.primaryAssetId
          textAttributeKey = [string]$observedFeature.textAttributeKey
          conditionIds = Get-StringArray -Value $observedFeature.conditionIds
          suppressed = [bool]$observedFeature.suppressed
        }
      }
      mismatches = @($mismatches)
    }
  }

  $cropReports = @()
  $cropDeltaCount = 0
  foreach ($expectedCrop in @($manifest.crops)) {
    $cropName = [string]$expectedCrop.name
    $comparison = [string](Get-OptionalProperty -Object $expectedCrop -Name 'comparison')
    if ([string]::IsNullOrWhiteSpace($comparison)) {
      $comparison = 'strict'
    }

    $observedCrop = Get-OptionalProperty -Object $observedCrops -Name $cropName
    $status = 'captured'
    $issues = New-Object System.Collections.Generic.List[string]

    if ($null -eq $observedCrop) {
      $status = 'missing'
      $issues.Add("crop '$cropName' missing from observation")
      if ($comparison -ne 'informational') {
        ++$cropDeltaCount
      }
    } else {
      $expectedNonEmpty = Get-OptionalProperty -Object $expectedCrop -Name 'expectedNonEmpty'
      if ($null -ne $expectedNonEmpty) {
        $observedNonEmpty = [int]$observedCrop.nonBackgroundPixels -gt 0
        if ([bool]$expectedNonEmpty -ne $observedNonEmpty) {
          $status = 'delta'
          $issues.Add(
            "expectedNonEmpty mismatch: expected=$expectedNonEmpty observed=$observedNonEmpty")
          ++$cropDeltaCount
        }
      }

      if ($comparison -eq 'informational') {
        $status = 'informational'
      }
    }

    $cropReports += [ordered]@{
      name = $cropName
      featureId = [string](Get-OptionalProperty -Object $expectedCrop -Name 'featureId')
      comparison = $comparison
      status = $status
      expectedVisualToken = [string](Get-OptionalProperty -Object $expectedCrop -Name 'expectedVisualToken')
      note = [string](Get-OptionalProperty -Object $expectedCrop -Name 'note')
      observed = if ($null -eq $observedCrop) {
        $null
      } else {
        [ordered]@{
          hash = [string]$observedCrop.hash
          nonBackgroundPixels = [int]$observedCrop.nonBackgroundPixels
          rect = [ordered]@{
            x = [int]$observedCrop.x
            y = [int]$observedCrop.y
            width = [int]$observedCrop.width
            height = [int]$observedCrop.height
          }
        }
      }
      issues = @($issues)
    }
  }

  if ($comparableFeatureCount -eq 0) {
    throw "Manifest $($manifestFile.Name) has no comparable features"
  }

  $observedSourceRcidCounts = New-CountMap -Values @(@($observation.features) | ForEach-Object { [string]$_.sourceRcid })
  $observedTableCounts = New-CountMap -Values @(@($observation.features) | ForEach-Object { [string]$_.tableName })
  $observedRuleCounts = New-CountMap -Values @(@($observation.features) | ForEach-Object { [string]$_.ruleId })

  $expectedSourceRcidCounts = New-CountMap -Values @(@($manifest.features) | ForEach-Object {
      if ([string](Get-OptionalProperty -Object $_ -Name 'comparison') -ne 'informational') {
        [string](Get-OptionalProperty -Object $_ -Name 'sourceRcid')
      }
    })
  $expectedTableCounts = New-CountMap -Values @(@($manifest.features) | ForEach-Object {
      if ([string](Get-OptionalProperty -Object $_ -Name 'comparison') -ne 'informational') {
        [string](Get-OptionalProperty -Object $_ -Name 'tableName')
      }
    })

  $sceneReport = [ordered]@{
    sceneId = $sceneId
    description = [string](Get-OptionalProperty -Object $manifest -Name 'description')
    normativeNote = "Engineering delta only; IHO S-52 / Annex A / S-64 remain authoritative."
    resourceSnapshot = Get-OptionalProperty -Object $manifest -Name 'resourceSnapshot'
    comparableFeatureCount = $comparableFeatureCount
    featureDeltaCount = $featureDeltaCount
    cropDeltaCount = $cropDeltaCount
    featureReports = $featureReports
    cropReports = $cropReports
    observedStatistics = [ordered]@{
      featureCount = @($observation.features).Count
      visibleLabelCount = [int]$observation.visibleLabelCount
      unicodeVisibleLabelCount = [int]$observation.unicodeVisibleLabelCount
      sourceRcidCounts = Convert-CountMapToArray -Map $observedSourceRcidCounts
      tableNameCounts = Convert-CountMapToArray -Map $observedTableCounts
      ruleIdCounts = Convert-CountMapToArray -Map $observedRuleCounts
    }
    expectedStatistics = [ordered]@{
      sourceRcidCounts = Convert-CountMapToArray -Map $expectedSourceRcidCounts
      tableNameCounts = Convert-CountMapToArray -Map $expectedTableCounts
    }
  }

  $sceneReportPath = Join-Path $OutDir "$sceneId.delta.json"
  Write-JsonFile -Path $sceneReportPath -Value $sceneReport

  $sceneSummaries += [ordered]@{
    sceneId = $sceneId
    featureDeltaCount = $featureDeltaCount
    cropDeltaCount = $cropDeltaCount
    reportPath = $sceneReportPath
  }

  $totalComparableFeatures += $comparableFeatureCount
  $totalFeatureDeltas += $featureDeltaCount
  $totalCropDeltas += $cropDeltaCount
}

$summary = [ordered]@{
  normativeNote = "Engineering delta only; OpenCPN is not a normative oracle."
  manifestDir = $ManifestDir
  observationDir = $ObservationDir
  outDir = $OutDir
  totalScenes = $manifestFiles.Count
  totalComparableFeatures = $totalComparableFeatures
  totalFeatureDeltas = $totalFeatureDeltas
  totalCropDeltas = $totalCropDeltas
  scenes = $sceneSummaries
}

$summaryPath = Join-Path $OutDir 'summary.json'
Write-JsonFile -Path $summaryPath -Value $summary

Write-Host "OpenCPN visual delta harness"
Write-Host "  normative note: engineering delta only; IHO S-52 / Annex A / S-64 remain authoritative"
Write-Host "  scenes: $($summary.totalScenes)"
Write-Host "  comparable features: $($summary.totalComparableFeatures)"
Write-Host "  feature deltas: $($summary.totalFeatureDeltas)"
Write-Host "  crop deltas: $($summary.totalCropDeltas)"
Write-Host "  summary: $summaryPath"
