#!/usr/bin/env python3
"""Original procedural skeletal-relief asset for the native Rack editor.
No external artwork. Requires numpy, scipy, scikit-image, numba and Pillow.
The editor consumes the baked transparent frame atlas, not this renderer.
"""
from pathlib import Path
import numpy as np
from scipy.interpolate import CubicSpline
from scipy.ndimage import gaussian_filter
from skimage.measure import marching_cubes
from numba import njit
from PIL import Image
import argparse

W,H=432,496
FRAMES=9
SCALE=155.0
CAM=4.1

class Mesh:
    def __init__(self): self.v=[];self.f=[];self.n=[];self.m=[];self.count=0
    def add(self,v,f,n,m):
        v=np.asarray(v,np.float32);f=np.asarray(f,np.int32);n=np.asarray(n,np.float32)
        self.v.append(v);self.f.append(f+self.count);self.n.append(n);self.m.append(np.full(len(f),m,np.int32));self.count+=len(v)
    def arrays(self):return tuple(np.concatenate(x) for x in (self.v,self.f,self.n,self.m))

# Smooth oriented surface with separate cap faces.
def tube(mesh,points,radii,mat=0,rings=12,sides=14,ridges=0.0):
    points=np.asarray(points,float);radii=np.asarray(radii,float)
    t=np.linspace(0,1,len(points));tt=np.linspace(0,1,rings)
    p=CubicSpline(t,points,axis=0,bc_type='natural')(tt)
    r=np.maximum(.001,np.interp(tt,t,radii))
    if ridges:r*=1+ridges*np.cos(tt*2*np.pi*17.)*(1-tt*.65)
    tangent=np.gradient(p,axis=0);tangent/=np.linalg.norm(tangent,axis=1,keepdims=True)
    ang=np.arange(sides)*2*np.pi/sides; vv=[];nn=[]
    for i in range(rings):
        u=np.cross(tangent[i],[0,0,1.]);
        if np.linalg.norm(u)<.1:u=np.cross(tangent[i],[0,1.,0])
        u/=np.linalg.norm(u);v=np.cross(tangent[i],u)
        n=np.cos(ang)[:,None]*u+np.sin(ang)[:,None]*v
        vv.append(p[i]+r[i]*n);nn.append(n)
    faces=[]
    for i in range(rings-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides;c=a+sides;d=b+sides
            faces.extend(((a,b,c),(b,d,c)))
    mesh.add(np.concatenate(vv),faces,np.concatenate(nn),mat)

def ellipsoid(mesh,c,rad,mat=0,tilt=0.):
    # Avoid pole artifacts with one continuous UV surface, enough samples for small bones.
    nphi,nt=24,14;theta=np.linspace(.001,np.pi-.001,nt);phi=np.arange(nphi)*2*np.pi/nphi
    p=np.stack(np.broadcast_arrays(np.sin(theta[:,None])*np.cos(phi),np.cos(theta[:,None]),np.sin(theta[:,None])*np.sin(phi)),axis=-1).reshape(-1,3)
    rad=np.asarray(rad);v=p*rad;n=p/rad;n/=np.linalg.norm(n,axis=1,keepdims=True)
    if tilt:
        rot=np.array([[np.cos(tilt),-np.sin(tilt),0],[np.sin(tilt),np.cos(tilt),0],[0,0,1.]])
        v=v@rot.T;n=n@rot.T
    v+=np.asarray(c)
    faces=[]
    for i in range(nt-1):
        for j in range(nphi):
            a=i*nphi+j;b=i*nphi+(j+1)%nphi;c=a+nphi;d=b+nphi;faces.extend(((a,b,c),(b,d,c)))
    mesh.add(v,faces,n,mat)

# Signed-distance carved skull, evaluated once. Eye sockets, nasal aperture,
# lower oral recess and zygomatic hollows are true negative volumes.
def skull_mesh(mesh):
    lo=np.array([-.65,-.25,-.32]);hi=np.array([.65,1.02,.76]);step=.009
    shape=np.ceil((hi-lo)/step).astype(int)+1
    x,y,z=[lo[i]+np.arange(shape[i],dtype=np.float32)*step for i in range(3)]
    x=x[:,None,None];y=y[None,:,None];z=z[None,None,:]
    def e(c,r,tilt=0.):
        a=x-c[0];b=y-c[1];d=z-c[2]
        if tilt:a,b=a*np.cos(tilt)+b*np.sin(tilt),-a*np.sin(tilt)+b*np.cos(tilt)
        return (np.sqrt((a/r[0])**2+(b/r[1])**2+(d/r[2])**2)-1)*min(r)
    def union(a,b,k=.045):
        h=np.maximum(k-np.abs(a-b),0)/k;return np.minimum(a,b)-h*h*k*.25
    s=e((0,.50,.08),(.472,.445,.34))
    s=union(s,e((0,.20,.19),(.36,.29,.30)))
    s=union(s,e((0,-.018,.335),(.295,.145,.23)))
    for side in (-1,1):
        s=union(s,e((side*.365,.095,.27),(.215,.135,.207),-side*.29))
        s=union(s,e((side*.235,.405,.35),(.267,.096,.155),side*.24),.025)
        s=union(s,e((side*.365,.29,.09),(.145,.24,.24)),.03)
        # Outer edge of the orbit higher, inner lower: threatening brow.
        s=np.maximum(s,-e((side*.238,.305,.445),(.213,.133,.287),side*.31))
        s=np.maximum(s,-e((side*.365,-.015,.415),(.126,.102,.218),side*.30))
        s=np.maximum(s,-e((side*.048,.105,.487),(.062,.12,.205),-side*.24))
    s=np.maximum(s,-e((0,-.188,.53),(.29,.138,.27)))
    # Fine nonuniform erosion, subdued enough to keep a continuous skull.
    s+=.0012*np.sin(x*81+y*27)*np.sin(y*91-z*58)*np.sin(z*77+x*44)
    # Narrow cranial sutures: irregular negative channels only in the frontal shell.
    for side in (-1,1):
        cx=side*(.10+.026*np.sin(y*33)+.014*np.sin(y*77));
        fissure=(np.abs(x-cx)-.0042)
        fissure=np.maximum(fissure,np.abs(y-.69)-.145)
        fissure=np.maximum(fissure,.28-z)
        s=np.maximum(s,-fissure)
    v,f,n,_=marching_cubes(s,0,spacing=(step,step,step),allow_degenerate=False)
    v=v+lo
    # skimage normals on ascending SDF point inward: orient them by sampling
    # the nearest field gradient, which points toward exterior.
    grad=np.gradient(s,step);idx=np.clip(np.rint((v-lo)/step).astype(int),0,shape-1)
    n=np.stack([g[idx[:,0],idx[:,1],idx[:,2]] for g in grad],axis=1);n/=np.maximum(1e-8,np.linalg.norm(n,axis=1,keepdims=True))
    mesh.add(v,f,n,0)

def build_static():
    m=Mesh()
    # Back spine and six separate curved ribs, no flat diagram lines.
    for i in range(12):ellipsoid(m,(0,-.20-i*.073,-.16),(.093,.052,.084),3)
    for side in (-1,1):
        X=lambda x:side*x
        for i in range(7):
            y=-.255-i*.104;w=.53-i*.033
            tube(m,[(X(.06),y+.04,-.17),(X(w*.72),y+.065,-.11),(X(w),y-.015,.10),(X(w*.72),y-.10,.27),(X(.07),y-.08,.30)], [.044,.048,.052,.039,.027],0,26,12)
        tube(m,[(X(.04),-.20,.22),(X(.34),-.14,.23),(X(.56),-.19,.12),(X(.68),-.14,.06)],[.065,.065,.074,.05],0,22)
        # Scapula spike and skeletal shoulder joint.
        tube(m,[(X(.45),-.23,-.02),(X(.64),-.15,-.02),(X(.74),.02,-.07)],[.12,.087,.002],3,18)
        ellipsoid(m,(X(.65),-.245,.10),(.155,.135,.13),0)
        tube(m,[(X(.65),-.25,.11),(X(.77),-.44,.20),(X(.87),-.64,.35)],[.081,.066,.088],0,18)
        ellipsoid(m,(X(.87),-.64,.35),(.105,.09,.095),0)
        tube(m,[(X(.85),-.65,.36),(X(.76),-.81,.60),(X(.66),-.91,.85)],[.068,.052,.060],0,18)
        tube(m,[(X(.93),-.65,.33),(X(.84),-.83,.59),(X(.74),-.93,.84)],[.050,.043,.049],3,18)
        # Massive bone horns, curved and segmented, with dark growth bands.
        tube(m,[(X(.34),.68,-.015),(X(.57),.75,.015),(X(.78),.88,.04),(X(.91),1.08,.03),(X(.88),1.25,-.03),(X(.78),1.33,-.07)], [.157,.145,.123,.086,.038,.001],3,96,22,.105)
        # Small horn roots join the cranium, avoiding floating cones.
        ellipsoid(m,(X(.37),.68,.04),(.16,.17,.14),0)
    # Front sternum and rib anchors.
    for i in range(7):ellipsoid(m,(0,-.34-i*.091,.29),(.057,.052,.039),0)
    head_start=len(m.v)
    skull_mesh(m)
    for side in (-1,1):
        # Small hot eyes deep inside true sockets.
        # Sharply angled heavy brows and thin molten slits, not round pupils.
        tube(m,[(side*.055,.342,.438),(side*.18,.386,.445),(side*.315,.435,.375),(side*.435,.412,.25)],[.048,.071,.077,.031],0,25,18)
        ellipsoid(m,(side*.222,.300,.275),(.087,.024,.031),1,side*.30)
        ellipsoid(m,(side*.217,.299,.302),(.046,.010,.013),4,side*.30)
        # Spiked cranial ridge, framing the angry face.
        tube(m,[(side*.30,.65,.14),(side*.39,.75,.15),(side*.43,.87,.08)],[.087,.055,.002],3,18,14)
    # Upper teeth along a curved dental arch; long outer canines.
    for i in range(10):
        x=(i-4.5)*.050;z=.520-.095*(abs(x)/.225)**1.5;y=-.106+.014*abs(i-4.5)
        l=.135 if i in (0,9) else .072+.014*(i%2)
        tube(m,[(x,y,z),(x,y-l*.53,z+.014),(x*.98,y-l,z+.002)],[.027,.022,.004],2,10,12)
    # Bring the skull toward the listener; keep the horns anchored to the cranium.
    for k in range(head_start,len(m.v)):
        m.v[k][:,0]*=1.12;m.v[k][:,1]=.27+(m.v[k][:,1]-.27)*1.08;m.v[k][:,2]+=.035
        m.n[k][:,0]/=1.12;m.n[k][:,1]/=1.08;m.n[k]/=np.maximum(np.linalg.norm(m.n[k],axis=1,keepdims=True),1e-8)
    return m

def add_jaw(m,open_amt):
    # Hinge-driven 3D jaw, rotating the chin forward and down, not a flat mouth overlay.
    # Y displacement grows toward the front of the mandible.
    def pose(p):
        x,y,z=p;w=np.clip((z-.11)/.44,0,1);return (x*1.12,.27+(y-open_amt*.22*w-.27)*1.08,z+open_amt*.035*w+.035)
    for side in (-1,1):
        X=lambda x:side*x
        points=[(X(.36),.015,.09),(X(.34),-.235,.17),(X(.235),-.36,.36),(X(.13),-.414,.49),(0,-.435,.53)]
        tube(m,[pose(p) for p in points],[.069,.067,.078,.070,.065],0,34,18)
    ellipsoid(m,pose((0,-.423,.53)),(.10,.063,.059),0)
    for i in range(10):
        x=(i-4.5)*.047;z=.523-.085*(abs(x)/.22)**1.4;y=-.386+.018*abs(i-4.5)
        l=.105 if i in (0,9) else .062+.012*(i%2)
        tube(m,[pose((x,y,z)),pose((x,y+l*.52,z+.008)),pose((x*.98,y+l,z+.001))],[.026,.022,.004],2,10,12)

def add_hands(m,amt):
    # Foreground joints and claws reach past the window frame in perspective.
    for side in (-1,1):
        X=lambda x:side*x;reach=amt*.027
        ellipsoid(m,(X(.69),-.935,.86+reach),(.18,.115,.095),3)
        for i in range(4):
            x=.49+i*.112;y=-.93+(.012*abs(i-1));z=.875+.015*(3-i)+reach
            ellipsoid(m,(X(x),y,z),(.052,.045,.049),0)
            tube(m,[(X(x),y,z),(X(x+.018),y-.074,z+.070),(X(x+.012),y-.135,z+.102)],[.052,.045,.044],0,12,14)
            ellipsoid(m,(X(x+.012),y-.135,z+.102),(.046,.042,.039),0)
            tube(m,[(X(x+.012),y-.135,z+.102),(X(x-.006),y-.195,z+.134),(X(x-.016),y-.227,z+.123)],[.038,.032,.025],0,12,14)
            tube(m,[(X(x-.016),y-.215,z+.124),(X(x-.025),y-.259,z+.144),(X(x-.065),y-.296,z+.102)],[.030,.022,.001],3,17,14)
        tube(m,[(X(.54),-.94,.84+reach),(X(.43),-.995,.88+reach),(X(.41),-1.045,.94+reach)],[.047,.039,.024],0,12)

def join(base,other):
    v,f,n,m=base;vv,ff,nn,mm=other.arrays();return np.concatenate([v,vv]),np.concatenate([f,ff+len(v)]),np.concatenate([n,nn]),np.concatenate([m,mm])

@njit(cache=True)
def raster(p,verts,norms,faces,mats,w,h):
    depth=np.full((h,w),-1e9,np.float32);pos=np.zeros((h,w,3),np.float32);ns=np.zeros((h,w,3),np.float32);mat=np.full((h,w),-1,np.int16)
    for fi in range(len(faces)):
        a,b,c=faces[fi];ax,ay,az=p[a];bx,by,bz=p[b];cx,cy,cz=p[c]
        den=(by-cy)*(ax-cx)+(cx-bx)*(ay-cy)
        if abs(den)<1e-7:continue
        xmin=max(0,int(np.floor(min(ax,bx,cx))));xmax=min(w-1,int(np.ceil(max(ax,bx,cx))))
        ymin=max(0,int(np.floor(min(ay,by,cy))));ymax=min(h-1,int(np.ceil(max(ay,by,cy))))
        for y in range(ymin,ymax+1):
            for x in range(xmin,xmax+1):
                l0=((by-cy)*(x+.5-cx)+(cx-bx)*(y+.5-cy))/den
                l1=((cy-ay)*(x+.5-cx)+(ax-cx)*(y+.5-cy))/den;l2=1-l0-l1
                if l0<0 or l1<0 or l2<0:continue
                d=l0*az+l1*bz+l2*cz
                if d<=depth[y,x]:continue
                depth[y,x]=d;mat[y,x]=mats[fi]
                for k in range(3):pos[y,x,k]=l0*verts[a,k]+l1*verts[b,k]+l2*verts[c,k];ns[y,x,k]=l0*norms[a,k]+l1*norms[b,k]+l2*norms[c,k]
    return depth,pos,ns,mat

def render(mesh):
    v,f,n,m=mesh
    # Shadow-map in key light space, including horns over skull and jaw over ribs.
    L=np.array([-.63,.70,.64]);L/=np.linalg.norm(L)
    U=np.cross([0,1,0],L);U/=np.linalg.norm(U);V=np.cross(L,U)
    ss=188.;sw=sh=576
    lp=np.stack([v@U*ss+sw/2,-v@V*ss+sh/2,v@L],axis=-1).astype(np.float32)
    shadow,*_=raster(lp,v,n,f,m,sw,sh)
    perspective=CAM/(CAM-v[:,2]);proj=np.stack([W/2+v[:,0]*perspective*SCALE,H*.465-v[:,1]*perspective*SCALE,v[:,2]],axis=-1).astype(np.float32)
    dep,p,norm,mat=raster(proj,v,n,f,m,W,H)
    mask=mat>=0; pp=p[mask];nn=norm[mask];nn/=np.maximum(np.linalg.norm(nn,axis=1,keepdims=True),1e-6)
    # Contact shadows with a compact PCF sample grid; no cast-shadow approximation pasted into the UI.
    sx=np.rint(pp@U*ss+sw/2).astype(int);sy=np.rint(-pp@V*ss+sh/2).astype(int);sd=pp@L
    vis=np.zeros(len(pp))
    for dx,dy in ((0,0),(-2,-2),(2,2),(-2,2),(2,-2),(-3,0),(3,0),(0,-3),(0,3)):
        d=shadow[np.clip(sy+dy,0,sh-1),np.clip(sx+dx,0,sw-1)]
        vis+=(sd>=d-.028)
    vis/=9
    N=nn;D=np.maximum(N@L,0);rimdir=np.array([.71,.14,-.68]);rimdir/=np.linalg.norm(rimdir);rim=np.maximum(N@rimdir,0)
    camera=np.array([0.,0.,CAM]);view=camera-pp;view/=np.maximum(1e-6,np.linalg.norm(view,axis=1,keepdims=True));half=L+view;half/=np.linalg.norm(half,axis=1,keepdims=True)
    spec=np.maximum(np.sum(N*half,axis=1),0)**32 * vis
    fres=(1-np.maximum(np.sum(N*view,axis=1),0))**2
    texture=.91+.045*np.sin(pp[:,0]*39+2*np.sin(pp[:,1]*31)+np.sin(pp[:,2]*45))+.033*np.sin(pp[:,1]*59+2*np.sin(pp[:,2]*36+pp[:,0]*23))+.019*np.sin(pp[:,2]*140+pp[:,0]*97)
    colors=np.array([[.48,.059,.049],[1.0,.14,.008],[.72,.19,.10],[.24,.025,.020],[1.,.58,.12]])[mat[mask]]
    rgb=colors*texture[:,None]*(.15+1.18*D[:,None]*vis[:,None])
    rgb+=np.array([.50,.048,.023])*rim[:,None]*(.50+.50*fres[:,None])
    rgb+=np.array([.72,.40,.27])*spec[:,None]*.43
    # Subtle red up-light, keeps shadows deep rather than flattening into orange.
    rgb+=np.array([.12,.014,.007])*np.maximum(-N[:,1],0)[:,None]
    emissive=mat[mask]==1;hot=mat[mask]==4
    rgb[emissive]=np.array([1.,.24,.025]);rgb[hot]=np.array([1.,.78,.26])
    # Small surface grain in camera space, color in sRGB-like artist units.
    rgb=np.clip(rgb,0,1);rgb=np.power(rgb,.82)
    image=np.zeros((H,W,4),np.uint8);image[mask,:3]=(rgb*255).astype(np.uint8);image[mask,3]=255
    eye=(mat==1)|(mat==4);bloom=gaussian_filter(eye.astype(np.float32),3.1)*1.1
    image[:,:,:3]=np.clip(image[:,:,:3].astype(np.float32)+bloom[:,:,None]*np.array([115,25,5],np.float32),0,255).astype(np.uint8)
    return Image.fromarray(image,'RGBA')

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',default='assets/demon_relief_atlas.png');ap.add_argument('--preview',action='store_true');a=ap.parse_args()
    base=build_static().arrays();atlas=Image.new('RGBA',(W*3,H*3));amounts=np.linspace(0,1,FRAMES)
    for i,amt in enumerate(amounts):
        if a.preview and i!=5:continue
        moving=Mesh();add_jaw(moving,float(amt));add_hands(moving,float(amt));im=render(join(base,moving));atlas.paste(im,((i%3)*W,(i//3)*H));print('frame',i,flush=True)
        if a.preview:im.save(a.output);return
    atlas.save(a.output,optimize=True)
if __name__=='__main__':main()
