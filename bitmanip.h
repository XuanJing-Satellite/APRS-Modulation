#ifndef __BITMANIP_H_
#define __BITMANIP_H_

#define BIT(x)	(1 << (x))

#define STB(p,b) ((p)|=(b))
#define CLB(p,b) ((p)&=~(b))
#define FLB(p,b) ((p)^=(b))
#define CHB(p,b) ((p) & (b))

#endif
