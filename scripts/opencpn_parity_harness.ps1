param(
  [Parameter(Mandatory = $true)]
  [string]$Reference,

  [Parameter(Mandatory = $true)]
  [string]$Observation
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

function Get-StringArray {
  param($Value)

  if ($null -eq $Value) {
    return @()
  }

  return @($Value | ForEach-Object { [string]$_ })
}

function Get-OptionalProperty {
  param($Object, [string]$Name)

  if ($null -eq $Object) {
    return $null
  }

  $property = $Object.PSObject.Properties[$Name]
  if ($null -eq $property) {
    return $null
  }

  return $property.Value
}

function Get-NumericProperty {
  param($Object, [string]$Name)

  if ($null -eq $Object) {
    return $null
  }

  $property = $Object.PSObject.Properties[$Name]
  if ($null -eq $property) {
    return $null
  }

  return [double]$property.Value
}

$referenceData = Read-JsonFile -Path $Reference
$observationData = Read-JsonFile -Path $Observation
$failures = New-Object System.Collections.Generic.List[string]

if ([string]$referenceData.sampleId -ne [string]$observationData.sampleId) {
  $failures.Add("sampleId mismatch: reference='$($referenceData.sampleId)' observation='$($observationData.sampleId)'")
}

$requiredChartIds = Get-StringArray -Value (Get-OptionalProperty -Object $referenceData -Name 'requiredChartIds')
$observedChartIds = Get-StringArray -Value (Get-OptionalProperty -Object $observationData -Name 'chartIds')
foreach ($chartId in $requiredChartIds) {
  if ($observedChartIds -notcontains $chartId) {
    $failures.Add("missing required chart id '$chartId'")
  }
}

$requiredRuleIds = Get-StringArray -Value (Get-OptionalProperty -Object $referenceData -Name 'requiredRuleIds')
$observedRuleIds = Get-StringArray -Value (Get-OptionalProperty -Object $observationData -Name 'ruleIds')
foreach ($ruleId in $requiredRuleIds) {
  if ($observedRuleIds -notcontains $ruleId) {
    $failures.Add("missing required rule id '$ruleId'")
  }
}

if ($null -ne (Get-OptionalProperty -Object $referenceData -Name 'minimums')) {
  foreach ($property in $referenceData.minimums.PSObject.Properties) {
    $expectedMinimum = [double]$property.Value
    $observedValue = Get-NumericProperty -Object $observationData -Name $property.Name
    if ($null -eq $observedValue) {
      $failures.Add("missing observed numeric field '$($property.Name)'")
      continue
    }

    if ($observedValue -lt $expectedMinimum) {
      $failures.Add(
        "field '$($property.Name)' below minimum: observed=$observedValue expected>=$expectedMinimum")
    }
  }
}

Write-Host "OpenCPN parity harness"
Write-Host "  sampleId: $($referenceData.sampleId)"
Write-Host "  normative note: engineering cross-check only; IHO S-52 / Annex A / S-64 remain authoritative"
Write-Host "  required chart ids: $([string]::Join(', ', $requiredChartIds))"
Write-Host "  observed chart ids: $([string]::Join(', ', $observedChartIds))"

if ($null -ne (Get-OptionalProperty -Object $referenceData -Name 'minimums')) {
  Write-Host "  minimum checks:"
  foreach ($property in $referenceData.minimums.PSObject.Properties) {
    $observedValue = Get-NumericProperty -Object $observationData -Name $property.Name
    Write-Host "    $($property.Name): observed=$observedValue expected>=$($property.Value)"
  }
}

if ($failures.Count -gt 0) {
  Write-Host "  result: FAIL"
  foreach ($failure in $failures) {
    Write-Host "    - $failure"
  }
  exit 1
}

Write-Host "  result: PASS"
