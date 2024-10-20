#version 410
out vec4 color;
in vec3 ipos, dpos;
uniform sampler3D vol;

void main(){
	ivec3 S = textureSize(vol,0)-1;
	ivec3 b = min(ivec3(ipos), S); vec3 p = ipos - b;
	vec3 a = vec3(dpos.x>0,dpos.y>0,dpos.z>0);
	vec2 yz = dpos.yz / dpos.x, xz = dpos.xz / dpos.y, xy = dpos.xy / dpos.z;
	float m = (ipos.x==0.||p.x==1.) ? .9 : (ipos.y==0.||p.y==1.) ? 1. : .8;
	color = texelFetch(vol, b, 0);
	color.rgb *= m;
	while(color.a < .997){
		vec2 iyz = p.yz + yz * (a.x-p.x);
		if(clamp(iyz, 0, 1) == iyz){ p.yz = iyz; m = .9; if((p.x=1.-a.x)>0){ if(--b.x<0)break;}else if(++b.x>S.x)break; }else{
			vec2 ixz = p.xz + xz * (a.y-p.y);
			if(clamp(ixz, 0, 1) == ixz){ p.xz = ixz; m = 1.; if((p.y=1.-a.y)>0){ if(--b.y<0)break;}else if(++b.y>S.y)break; }else{
				vec2 ixy = p.xy + xy * (a.z-p.z);
				if(clamp(ixy, 0, 1) == ixy){ p.xy = ixy; m = .8; if((p.z=1.-a.z)>0){ if(--b.z<0)break;}else if(++b.z>S.z)break; }else break;
			}
		}
		vec4 c = texelFetch(vol, b, 0); c.rgb *= m;
		color += (1.-color.a)*c;
	}
}