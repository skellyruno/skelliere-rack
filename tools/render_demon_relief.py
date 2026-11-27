#!/usr/bin/env python3
"""Original procedural skeletal-relief asset for the native Rack editor.
No external artwork. Requires numpy, scipy, scikit-image, numba and Pillow.
The editor consumes the baked transparent frame atlas, not this renderer.
"""
from pathlib import Path
import numpy as np
from scipy.interpolate import CubicSpline
from scipy.ndimage import gaussian_filter, maximum_filter
from skimage.measure import marching_cubes
from numba import njit
from PIL import Image
import argparse

W,H=432,496
FRAMES=9
SCALE=157.0
CAM=4.1

class Mesh:
    def __init__(self): self.v=[];self.f=[];self.n=[];self.m=[];self.count=0
    def add(self,v,f,n,m):
        v=np.asarray(v,np.float32);f=np.asarray(f,np.int32);n=np.asarray(n,np.float32)
        self.v.append(v);self.f.append(f+self.count);self.n.append(n);self.m.append(np.full(len(f),m,np.int32));self.count+=len(v)
    def arrays(self):return tuple(np.concatenate(x) for x in (self.v,self.f,self.n,self.m))

# Smooth oriented surface with separate cap faces.
def tube(mesh,points,radii,mat=0,rings=12,sides=14,ridges=0.0,aspect=1.0):
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
        # Elliptical sections flatten the mandible and ribs, rather than
        # rendering skeletal anatomy as a set of circular plastic rods.
        cross=np.cos(ang)[:,None]*u*aspect+np.sin(ang)[:,None]*v
        normal=np.cos(ang)[:,None]*u/aspect+np.sin(ang)[:,None]*v
        normal/=np.linalg.norm(normal,axis=1,keepdims=True)
        if mat in (0,2,3):
            # Small anisotropic longitudinal ridges and noncircular cortical shafts.
            # Local variations remain fixed in object space and through jaw animation.
            phase=points[0,0]*7+points[0,1]*11
            profile=1+.022*np.sin(3*ang+phase+tt[i]*3)+.014*np.cos(5*ang-phase+tt[i]*6)
        else:profile=np.ones(sides)
        vv.append(p[i]+r[i]*cross*profile[:,None]);nn.append(normal)
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
    s=e((0,.47,.08),(.436,.445,.35))
    s=union(s,e((0,.20,.19),(.32,.278,.30)))
    s=union(s,e((0,.64,.19),(.09,.135,.13)),.035)
    s=union(s,e((0,-.018,.335),(.295,.145,.23)))
    s=union(s,e((0,.245,.326),(.082,.184,.107)),.025)
    for side in (-1,1):
        s=union(s,e((side*.345,.095,.24),(.178,.087,.185),-side*.42))
        s=union(s,e((side*.235,.405,.35),(.246,.075,.155),side*.24),.045)
        s=union(s,e((side*.13,.115,.32),(.108,.14,.125),-side*.22),.025)
        # Temporal fossa and infraorbital depression give the skull real bony planes.
        s=np.maximum(s,-e((side*.47,.34,.19),(.093,.145,.20),side*.15))
        s=np.maximum(s,-e((side*.19,.15,.48),(.080,.035,.082),side*.16))
        s=union(s,e((side*.365,.29,.09),(.145,.24,.24)),.03)
        # Outer edge of the orbit higher, inner lower: threatening brow.
        s=np.maximum(s,-e((side*.228,.305,.445),(.178,.139,.40),side*.32))
        s=np.maximum(s,-e((side*.345,-.015,.415),(.119,.085,.245),side*.30))
        # Pear-shaped nasal aperture with a narrow bony septum, carved into
        # the maxilla instead of two long cylindrical holes.
        s=np.maximum(s,-e((side*.037,.095,.487),(.057,.103,.205),-side*.30))
        s=np.maximum(s,-e((side*.049,.035,.498),(.064,.055,.19),-side*.12))
    s=np.maximum(s,-e((0,-.188,.53),(.29,.138,.27)))
    # Fine nonuniform erosion, subdued enough to keep a continuous skull.
    s+=.0011*np.sin(x*81+y*27)*np.sin(y*91-z*58)*np.sin(z*77+x*44)
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
            y=-.255-i*.104;w=.67-i*.040
            tube(m,[(X(.06),y+.04,-.17),(X(w*.72),y+.065,-.11),(X(w),y-.015,.10),(X(w*.72),y-.10,.27),(X(.07),y-.08,.30)], [.038,.048,.052,.041,.025],0,38,18,aspect=.70)
        tube(m,[(X(.04),-.20,.22),(X(.34),-.14,.23),(X(.56),-.19,.12),(X(.68),-.14,.06)],[.065,.065,.074,.05],0,22)
        # Scapula spike and skeletal shoulder joint.
        tube(m,[(X(.45),-.23,-.02),(X(.64),-.15,-.02),(X(.87),.13,-.07)],[.14,.092,.002],3,18)
        ellipsoid(m,(X(.70),-.245,.10),(.210,.182,.182),0)
        tube(m,[(X(.70),-.25,.11),(X(.74),-.32,.15),(X(.78),-.44,.20),(X(.84),-.57,.30),(X(.87),-.64,.35)],[.130,.113,.075,.078,.105],0,36,22,aspect=.85)
        ellipsoid(m,(X(.87),-.64,.35),(.105,.09,.095),0)
        tube(m,[(X(.85),-.65,.36),(X(.80),-.73,.47),(X(.76),-.81,.60),(X(.70),-.87,.76),(X(.66),-.91,.85)],[.083,.064,.044,.052,.068],0,34,20,aspect=.78)
        tube(m,[(X(.93),-.65,.33),(X(.84),-.83,.59),(X(.74),-.93,.84)],[.050,.043,.049],3,18)
        # Massive bone horns, curved and segmented, with dark growth bands.
        tube(m,[(X(.34),.68,-.015),(X(.57),.75,.015),(X(.78),.88,.04),(X(.94),1.04,.05),(X(.87),1.24,-.01),(X(.70),1.38,.02)], [.177,.165,.139,.096,.040,.001],3,96,22,.105)
        # Overlapping scapular ridges and clavicle spurs broaden the silhouette.
        tube(m,[(X(.26),-.18,.20),(X(.50),-.08,.22),(X(.75),-.08,.15),(X(.83),.07,.05)],[.060,.081,.074,.005],0,24,18)
        tube(m,[(X(.58),-.26,.19),(X(.75),-.20,.24),(X(.84),-.13,.18)],[.071,.093,.003],3,18,16)
        tube(m,[(X(.70),-.31,.19),(X(.81),-.47,.28),(X(.89),-.58,.36)],[.035,.040,.049],0,20,16)
        # Layered shoulder armour, without extending the rib cage.
        for i in range(3):
            tube(m,[(X(.49+i*.075),-.22-i*.065,.16),(X(.68+i*.055),-.26-i*.075,.27),(X(.76+i*.035),-.34-i*.075,.23)],[.055,.075,.006],0,23,16)
        # Small horn roots join the cranium, avoiding floating cones.
        ellipsoid(m,(X(.37),.68,.04),(.16,.17,.14),0)
    # Exposed lumbar spine below the last rib. Preserve the approved head,
    # shoulders, rib cage and hands. Each vertebra has a body, disc and
    # tapered transverse processes; no additional or stretched ribs.
    for i in range(5):
        yy=-.94-i*.062; zz=.045+.014*i; xx=.009*np.sin(i*.9)
        ellipsoid(m,(xx,yy,zz),(.077-i*.003,.036,.063-i*.002),0)
        ellipsoid(m,(xx,yy-.031,zz),(.054,.015,.048),3)
        for side in (-1,1):
            tube(m,[(xx+side*.035,yy,zz),(xx+side*.083,yy-.009,zz-.012),(xx+side*.105,yy-.023,zz-.026)],[.025,.020,.003],0,12,12,aspect=.76)
        tube(m,[(xx,yy,zz-.028),(xx,yy-.018,zz-.073),(xx,yy-.037,zz-.091)],[.025,.018,.003],3,10,12)
    # Front sternum and rib anchors.
    tube(m,[(0,-.25,.28),(0,-.48,.32),(0,-.80,.31),(0,-.90,.26)],[.066,.078,.060,.024],0,40,18)
    for i in range(7):ellipsoid(m,(0,-.34-i*.091,.32),(.063,.050,.031),0)
    # Broaden the shoulder girdle, keep the ribs and sternum in perspective.
    for k in range(len(m.v)):
        vv=m.v[k]; factor=1.0+.08*np.clip((-vv[:,1]+.12)/.65,0,1)*np.clip((vv[:,1]+1.18)/.40,0,1)
        vv[:,0]*=factor
        m.n[k][:,0]/=factor
        m.n[k]/=np.maximum(np.linalg.norm(m.n[k],axis=1,keepdims=True),1e-8)
    head_start=len(m.v)
    skull_mesh(m)
    for side in (-1,1):
        # Small hot eyes deep inside true sockets.
        # Sharply angled heavy brows and thin molten slits, not round pupils.
        tube(m,[(side*.055,.355,.405),(side*.18,.393,.402),(side*.315,.435,.350),(side*.435,.412,.235)],[.022,.035,.033,.009],0,30,22,aspect=.84)
        ellipsoid(m,(side*.222,.300,.275),(.098,.024,.028),1,side*.30)
        ellipsoid(m,(side*.217,.299,.302),(.046,.010,.013),4,side*.30)
        # Sharper zygomatic spurs and brow ridge, receding in the temple.
        tube(m,[(side*.25,.135,.35),(side*.37,.11,.34),(side*.43,.06,.22)],[.038,.049,.009],0,20,16,aspect=.80)
        tube(m,[(side*.11,.73,.25),(side*.24,.79,.19),(side*.34,.86,.10)],[.056,.065,.009],3,18,16)
        # Spiked cranial ridge, framing the angry face.
        tube(m,[(side*.30,.65,.14),(side*.39,.75,.15),(side*.43,.87,.08)],[.087,.055,.002],3,18,14)
    # Upper teeth along a curved dental arch; long outer canines.
    for i in range(10):
        x=(i-4.5)*.050;z=.520-.095*(abs(x)/.225)**1.5;y=-.106+.014*abs(i-4.5)
        l=.126 if i in (0,9) else .064+.012*np.cos(i*1.8)**2
        tube(m,[(x,y,z),(x,y-l*.53,z+.014),(x*.98,y-l,z+.002)],[.028,.024,.006],2,14,16)
    # Bring the skull toward the listener; keep the horns anchored to the cranium.
    for k in range(head_start,len(m.v)):
        m.v[k][:,0]*=1.12;m.v[k][:,1]=.27+(m.v[k][:,1]-.27)*1.13;m.v[k][:,2]+=.07
        m.n[k][:,0]/=1.12;m.n[k][:,1]/=1.13;m.n[k]/=np.maximum(np.linalg.norm(m.n[k],axis=1,keepdims=True),1e-8)
    return m

def add_jaw(m,open_amt):
    # Hinge-driven 3D jaw, rotating the chin forward and down, not a flat mouth overlay.
    # Y displacement grows toward the front of the mandible.
    def pose(p):
        x,y,z=p;w=np.clip((z-.11)/.44,0,1);return (x*1.12,.27+(y-open_amt*.24*w-.27)*1.13,z+open_amt*.083*w+.07)
    for side in (-1,1):
        X=lambda x:side*x
        points=[(X(.33),.015,.09),(X(.31),-.18,.17),(X(.226),-.31,.36),(X(.122),-.365,.49),(0,-.387,.53)]
        tube(m,[pose(p) for p in points],[.056,.055,.068,.063,.059],0,42,20,aspect=.64)
    ellipsoid(m,pose((0,-.383,.53)),(.10,.058,.047),0)
    for i in range(10):
        x=(i-4.5)*.047;z=.523-.085*(abs(x)/.22)**1.4;y=-.346+.018*abs(i-4.5)
        l=.098 if i in (0,9) else .060+.011*np.sin(i*1.3)**2
        tube(m,[pose((x,y,z)),pose((x,y+l*.52,z+.008)),pose((x*.98,y+l,z+.001))],[.026,.024,.006],2,14,16)

def add_hands(m,amt):
    # Foreground joints and claws reach past the window frame in perspective.
    for side in (-1,1):
        X=lambda x:side*x;reach=amt*.16
        ellipsoid(m,(X(.69),-.935,.86+reach),(.225,.123,.112),3)
        # Four separate metacarpal ridges, merging into the existing hand grip.
        for k in range(4):
            px=.49+k*.112
            tube(m,[(X(.70+(px-.70)*.32),-.855,.825+reach),(X(px),-.899,.912+reach),(X(px),-.939,.927+reach)],[.043,.048,.032],0,18,16,aspect=.72)
        for i in range(4):
            x=.49+i*.112;y=-.93+(.012*abs(i-1));z=.875+.015*(3-i)+reach
            ellipsoid(m,(X(x),y,z),(.052,.045,.049),0)
            tube(m,[(X(x),y,z),(X(x+.018),y-.074,z+.070),(X(x+.012),y-.135,z+.102)],[.052,.045,.044],0,12,14)
            ellipsoid(m,(X(x+.012),y-.135,z+.102),(.046,.042,.039),0)
            tube(m,[(X(x+.012),y-.135,z+.102),(X(x-.006),y-.195,z+.134),(X(x-.016),y-.227,z+.123)],[.038,.032,.025],0,12,14)
            tube(m,[(X(x-.016),y-.215,z+.124),(X(x-.025),y-.259,z+.144),(X(x-.065),y-.309,z+.112)],[.030,.022,.001],3,17,14)
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
    L=np.array([-.78,.55,.55]);L/=np.linalg.norm(L)
    U=np.cross([0,1,0],L);U/=np.linalg.norm(U);V=np.cross(L,U)
    ss=272.;sw=sh=832
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
        vis+=(sd>=d-.018)
    vis/=9
    # Screen-space cavity occlusion at multiple radii. Only count nearby geometry
    # in front of each visible surface; the transparent background never occludes.
    occ=np.zeros_like(dep,dtype=np.float32)
    for radius,weight in ((2,.38),(5,.27),(11,.18)):
        near=maximum_filter(dep,size=2*radius+1)
        diff=np.where(mask,np.maximum(0,near-dep-.018),0)
        occ+=weight*np.clip(diff/.27,0,1)
    ao=1.-np.clip(occ[mask],0,.58)
    N=nn;D=np.maximum(N@L,0);rimdir=np.array([.71,.14,-.68]);rimdir/=np.linalg.norm(rimdir);rim=np.maximum(N@rimdir,0)
    camera=np.array([0.,0.,CAM]);view=camera-pp;view/=np.maximum(1e-6,np.linalg.norm(view,axis=1,keepdims=True));half=L+view;half/=np.linalg.norm(half,axis=1,keepdims=True)
    spec=np.maximum(np.sum(N*half,axis=1),0)**25 * vis
    fres=(1-np.maximum(np.sum(N*view,axis=1),0))**2
    # Pitted, ridged bone rather than polished plastic. Multi-frequency material
    # variation is spatially stable over all nine animation frames.
    pore=np.sin(pp[:,0]*113+np.sin(pp[:,1]*57))*np.sin(pp[:,1]*127+np.sin(pp[:,2]*63))*np.sin(pp[:,2]*131+pp[:,0]*61)
    texture=.84+.075*np.sin(pp[:,0]*39+2*np.sin(pp[:,1]*31)+np.sin(pp[:,2]*45))+.045*np.sin(pp[:,1]*59+2*np.sin(pp[:,2]*36+pp[:,0]*23))+.027*np.sin(pp[:,2]*140+pp[:,0]*97)-.10*np.maximum(pore,0)**3
    colors=np.array([[.56,.126,.101],[1.0,.14,.008],[.71,.24,.15],[.22,.028,.020],[1.,.58,.12]])[mat[mask]]
    vein=np.exp(-22*np.abs(np.sin(pp[:,0]*21+pp[:,1]*14+2.5*np.sin(pp[:,2]*17+pp[:,1]*8))))
    texture*=1-.14*vein
    # Stable irregular mineral grain in object space; no frame-dependent noise.
    def grain(p,freq):
        q=p*freq;i=np.floor(q).astype(np.int32);f=q-i;f=f*f*(3.-2.*f)
        acc=np.zeros(len(p))
        for a in range(2):
            for b in range(2):
                for c in range(2):
                    h=np.uint32(i[:,0]+a)*np.uint32(0x9e3779b9)^np.uint32(i[:,1]+b)*np.uint32(0x85ebca6b)^np.uint32(i[:,2]+c)*np.uint32(0xc2b2ae35)
                    h^=h>>np.uint32(16);h*=np.uint32(0x7feb352d);h^=h>>np.uint32(15)
                    acc+=(h.astype(np.float64)/4294967295.)*(f[:,0] if a else 1-f[:,0])*(f[:,1] if b else 1-f[:,1])*(f[:,2] if c else 1-f[:,2])
        return acc
    # Bone is a rough dielectric, not painted plastic. Work in linear light,
    # vary mineral density and perturb the geometric normals with stable 3D
    # microstructure. No frame-dependent noise or modification to silhouette.
    mottled=.50*grain(pp,12)+.30*grain(pp,38)+.20*grain(pp,95)
    coarse=grain(pp,9);fine=grain(pp,120)
    def relief(p):
        return .50*grain(p,27)+.35*grain(p,68)+.15*grain(p,135)
    delta=.0025
    grad=np.stack([(relief(pp+np.eye(3)[i]*delta)-relief(pp-np.eye(3)[i]*delta))/(2*delta) for i in range(3)],axis=1)
    grad-=np.sum(grad*N,axis=1,keepdims=True)*N
    N=N-.0037*grad;N/=np.maximum(np.linalg.norm(N,axis=1,keepdims=True),1e-6)
    D=np.maximum(N@L,0);rim=np.maximum(N@rimdir,0)
    nv=np.maximum(np.sum(N*view,axis=1),0);nh=np.maximum(np.sum(N*half,axis=1),0)
    # GGX microfacet lobe with varying roughness, Schlick Fresnel and Smith masking.
    rough=np.clip(.65+.20*(mottled-.5)+.06*vein,.50,.82)
    alpha2=rough**4
    ndist=alpha2/(np.pi*np.maximum(1e-5,(nh*nh*(alpha2-1)+1)**2))
    k=(rough+1)**2/8
    gv=nv/np.maximum(nv*(1-k)+k,1e-6);gl=D/np.maximum(D*(1-k)+k,1e-6)
    vh=np.maximum(np.sum(view*half,axis=1),0)
    fresnel=.035+.965*(1-vh)**5
    glossy=ndist*gv*gl*fresnel/np.maximum(4*nv*D,1e-5)*D*vis
    # Red environmental illumination, warm mineral highlights, dark crevices.
    # Material-specific linear albedos retain differentiated enamel, bone,
    # horn and joint surfaces instead of a uniform red diffuse tint.
    bone=np.array([[.39,.082,.043],[1.,.14,.008],[.48,.21,.115],[.095,.021,.014],[1.,.58,.12]])[mat[mask]]
    texture*=.72+.52*mottled
    bone*=np.column_stack((.84+.28*coarse,.78+.40*coarse,.68+.49*coarse))
    bone*=1-.26*np.maximum(pore,0)[:,None]**3
    # Fine stained fissures and dark mineral inclusions on the cortical surface.
    ridge=np.abs(np.sin(pp[:,1]*34+2.8*grain(pp,16)+pp[:,0]*12))
    stain=np.exp(-36*ridge)*np.clip((mottled-.27)*2.5,0,1)
    bone*=1-.23*stain[:,None]
    key=np.array([1.32,.46,.34]);bounce=np.array([.22,.018,.012]);edge=np.array([1.15,.12,.06])
    shade=(.012+1.61*D*vis)*ao**1.75
    rgb=bone*texture[:,None]*shade[:,None]*key
    rgb+=bone*bounce[None,:]*(.16+.70*np.maximum(-N[:,1],0))[:,None]*ao[:,None]
    rgb+=bone*edge[None,:]*rim[:,None]*(.24+.76*(1-nv[:,None])**2)
    rgb+=np.array([1.18,.74,.48])[None,:]*glossy[:,None]*(.87+.24*fine[:,None])*ao[:,None]
    # Low-strength fill to retain the skull's readable silhouette and hand joints.
    fill=np.maximum(N@np.array([-.38,-.26,.72]),0)
    rgb+=bone*np.array([.09,.025,.016])*fill[:,None]*ao[:,None]
    # Camera transfer: gentle highlight shoulder and standard sRGB encoding.
    rgb=rgb/(1+.27*rgb)
    rgb=np.where(rgb<=.0031308,12.92*rgb,1.055*np.maximum(rgb,0)**(1/2.4)-.055)
    emissive=mat[mask]==1;hot=mat[mask]==4
    rgb[emissive]=np.array([1.,.24,.025]);rgb[hot]=np.array([1.,.78,.26])
    # Small surface grain in camera space, color in sRGB-like artist units.
    rgb=np.clip(rgb,0,1)
    image=np.zeros((H,W,4),np.uint8);image[mask,:3]=(rgb*255).astype(np.uint8);image[mask,3]=255
    eye=(mat==1)|(mat==4);bloom=gaussian_filter(eye.astype(np.float32),3.1)*1.45
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
