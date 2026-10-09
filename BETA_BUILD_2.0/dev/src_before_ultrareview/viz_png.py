import numpy as np, subprocess, tempfile, math
from PIL import Image
T=tempfile.mkdtemp(); rng=np.random.default_rng(2)
def cfg_write(d):
    with open(f"{T}/c.txt","w") as f:
        for k,v in d.items():
            vals=v if isinstance(v,(list,tuple,np.ndarray)) else [v]
            f.write(k+" "+" ".join(repr(float(x)) for x in vals)+" ;\n")
def raster(d):
    cfg_write(d); subprocess.run(["./test_gfx","raster",f"{T}/c.txt",f"{T}/o.f32"],check=True)
    return np.fromfile(f"{T}/o.f32",dtype=np.float32).reshape(int(d["vh"]),int(d["vw"]),4)
def over(img, bg=(0.07,0.11,0.13)):
    a=img[...,3:4]; return img[...,:3]+np.array(bg)*(1-a)
lv=np.clip(0.5+0.45*np.sin(np.linspace(0,6,32))+0.1*rng.standard_normal(32),0.02,1)
pk=np.minimum(1,lv+0.12)
tiles=[]
base=dict(vw=340,vh=170,bx=10.4,by=12.6,barW=6,gap=4,maxSize=140,idle=4,bars=32,vertical=0,levels=lv.tolist(),peaks=pk.tolist(),
          c1=(0.08,1,0.64,1),grad1=(0.08,0.72,0.65,1),c2=(0.78,0.11,0.2,1),peakColor=(1,0.56,0.56,1),capT=2,flags=1)
tiles.append(over(raster(dict(base,anchor=2,radii=[3,3,3,3],colorMode=1,passes=[1,32,2,32]))))
tiles.append(over(raster(dict(base,anchor=1,radii=[5,5,0,0],colorMode=7,rainbowBase=40,passes=[1,32,2,32]))))
ms=math.ceil(140/10)+1
tiles.append(over(raster(dict(base,anchor=2,dotRadii=[3,3,3,3],colorMode=2,passes=[3,32*ms]))))
tiles.append(over(raster(dict(base,vw=340,vh=170,cx=170,cy=85,maxSize=70,bars=48,levels=np.tile(lv,2)[:48].tolist(),colorMode=1,barW=3,passes=[4,48]))))
img=np.concatenate([np.concatenate(tiles[:2],axis=1),np.concatenate(tiles[2:],axis=1)],axis=0)
Image.fromarray((np.clip(img,0,1)*255).astype(np.uint8)).resize((img.shape[1]*2,img.shape[0]*2),Image.NEAREST).save("../shader_preview.png")
