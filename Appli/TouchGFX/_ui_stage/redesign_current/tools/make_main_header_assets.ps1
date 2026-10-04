Add-Type -AssemblyName System.Drawing

$assets = Join-Path $PSScriptRoot '..\Appli\TouchGFX\assets\images'
$pairs = @(
    @('btn_output_v2_off_touch_166x56.png', 'main_header_btn_rel_140x52.png'),
    @('btn_output_v2_on_touch_166x56.png',  'main_header_btn_sel_140x52.png')
)

foreach ($pair in $pairs) {
    $sourcePath = Join-Path $assets $pair[0]
    $targetPath = Join-Path $assets $pair[1]
    $source = [System.Drawing.Image]::FromFile($sourcePath)
    try {
        $target = New-Object System.Drawing.Bitmap 140, 52
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($target)
            try {
                $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
                $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $graphics.DrawImage($source, 0, 0, 140, 52)
            } finally {
                $graphics.Dispose()
            }
            $target.Save($targetPath, [System.Drawing.Imaging.ImageFormat]::Png)
        } finally {
            $target.Dispose()
        }
    } finally {
        $source.Dispose()
    }
}
