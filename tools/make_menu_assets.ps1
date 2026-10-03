Add-Type -AssemblyName System.Drawing
$assets = Join-Path $PSScriptRoot '..\Appli\TouchGFX\assets\images'
$pairs = @(
    @('btn_menu_released_350x120.png', 'btn_menu_released_350x78.png'),
    @('btn_menu_pressed_350x120.png',  'btn_menu_pressed_350x78.png'),
    @('btn_menu_released_350x120.png', 'btn_sequence_row_released_740x56.png'),
    @('btn_menu_pressed_350x120.png',  'btn_sequence_row_selected_740x56.png')
)
foreach ($pair in $pairs) {
    $source = [System.Drawing.Image]::FromFile((Join-Path $assets $pair[0]))
    try {
        $width = if ($pair[1] -like '*740x56*') { 740 } else { 350 }
        $height = if ($pair[1] -like '*740x56*') { 56 } else { 78 }
        $target = New-Object System.Drawing.Bitmap $width, $height
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($target)
            try {
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.DrawImage($source, 0, 0, $width, $height)
            } finally { $graphics.Dispose() }
            $target.Save((Join-Path $assets $pair[1]), [System.Drawing.Imaging.ImageFormat]::Png)
        } finally { $target.Dispose() }
    } finally { $source.Dispose() }
}
