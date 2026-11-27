#include <assert.h>
#include <stdio.h>
#include "house_shape.h"
int main(void){
 for(int side=0;side<2;side++)for(int end=0;end<2;end++)for(int w=1;w<=3;w++){
  HouseShapeRect a={5,6,16,13},b={side?5:5+w,6,side?16-w:16,13};
  HouseShapeRect g={side?16-w:5,end?6:8,side?16:5+w,end?11:13};
  int c=house_shape_encode(a,b,g); assert(c>0&&c<=12);
  HouseShapeRect bb,gg;assert(house_shape_decode(a,c,&bb,&gg));
  assert(house_shape_equal(b,bb)&&house_shape_equal(g,gg));
  for(int z=5;z<14;z++)for(int x=4;x<17;x++){
   int v=house_shape_contains(bb,x,z)+house_shape_contains(gg,x,z);
   assert(v<=1);assert(v==(house_shape_contains(b,x,z)||house_shape_contains(g,x,z)));
  }
 }
 HouseShapeRect a={5,6,16,13},b={5,6,16,13},g={0};assert(!house_shape_encode(a,b,g));
 puts("L-shaped skyline cache: all orientations and wing widths passed");
}
