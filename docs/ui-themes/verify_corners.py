from pathlib import Path
from PIL import Image,ImageOps,ImageChops
root=Path(__file__).resolve().parents[2]/'Appli/TouchGFX/screenshots/theme-selftest'
out=Path(__file__).parent/'corners-previews';out.mkdir(exist_ok=True)
def verify(image,rect,radius,label):
    x,y,w,h=rect
    tl=image.crop((x,y,x+radius,y+radius))
    tr=ImageOps.mirror(image.crop((x+w-radius,y,x+w,y+radius)))
    bl=ImageOps.flip(image.crop((x,y+h-radius,x+radius,y+h)))
    br=ImageOps.flip(ImageOps.mirror(image.crop((x+w-radius,y+h-radius,x+w,y+h))))
    assert all(ImageChops.difference(tl,c).getbbox() is None for c in [tr,bl,br]),label+' corners differ'
    print('PASS four-corner pixel symmetry:',label)
for stage,mode in [(1,'dark'),(6,'light')]:
    image=Image.open(root/f'theme-{stage:02}.bmp').convert('RGB');image.save(out/f'appearance-{mode}.png')
    for rect,radius,label in [((16,92,768,126),12,'panel'),((300,122,88,48),8,'light selector'),((396,122,88,48),8,'dark selector'),((300,294,88,48),8,'blue accent'),((548,8,112,48),8,'header shutdown')]:
        verify(image,rect,radius,mode+' '+label)
    assert image.getpixel((300,122))==image.getpixel((298,146)),mode+' selector corner does not match its panel'
    assert image.getpixel((300,294))==image.getpixel((298,318)),mode+' accent corner does not match its panel'
    print('PASS panel backdrop:',mode)
image=Image.open(root/'theme-23.bmp').convert('RGB');image.save(out/'main-dark.png')
for rect,radius,label in [((16,80,484,151),12,'main voltage card'),((524,116,76,48),8,'key 1'),((608,116,76,48),8,'key 2'),((608,332,160,48),8,'apply')]:
    verify(image,rect,radius,label)
    if label!='main voltage card':assert image.getpixel((rect[0],rect[1]))==image.getpixel((rect[0]-2,rect[1]+rect[3]//2)),label+' corner has wrong backdrop'
print('PASS all: symmetric antialiased corners and correct light/dark parent backgrounds')
