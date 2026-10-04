Add-Type -AssemblyName System.Drawing

function New-RoundedPath([System.Drawing.RectangleF]$rect, [float]$radius) {
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $diameter = 2 * $radius
    $path.AddArc($rect.X, $rect.Y, $diameter, $diameter, 180, 90)
    $path.AddArc($rect.Right - $diameter, $rect.Y, $diameter, $diameter, 270, 90)
    $path.AddArc($rect.Right - $diameter, $rect.Bottom - $diameter, $diameter, $diameter, 0, 90)
    $path.AddArc($rect.X, $rect.Bottom - $diameter, $diameter, $diameter, 90, 90)
    $path.CloseFigure()
    return $path
}

function Write-SelectionAsset([string]$path) {
    $directory = Split-Path -Parent $path
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
    $bitmap = New-Object System.Drawing.Bitmap 484, 151, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.Clear([System.Drawing.Color]::Transparent)
            $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
            $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(255, 42, 198, 218)), 2
            try {
                $pen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
                $outline = New-RoundedPath ([System.Drawing.RectangleF]::new(1, 1, 482, 149)) 10
                try { $graphics.DrawPath($pen, $outline) } finally { $outline.Dispose() }
                $graphics.DrawLine($pen, 307, 12, 307, 139)
            } finally { $pen.Dispose() }
        } finally { $graphics.Dispose() }
        $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally { $bitmap.Dispose() }
}

$root = Split-Path -Parent $PSScriptRoot
Write-SelectionAsset (Join-Path $root 'Appli\TouchGFX\assets\images\setpoint_selected_484x151.png')
Write-SelectionAsset (Join-Path $root 'Appli\TouchGFX\_ui_stage\redesign_current\Appli\TouchGFX\assets\images\setpoint_selected_484x151.png')
