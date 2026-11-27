#include <assert.h>
#include <stdio.h>
#include "arrival_plan.h"
static unsigned char field[18][18];
static int clear(int x,int y){return field[y][x]!=0;}
int main(void){
    uint16_t used[16];
    for(int i=0;i<18;i++)for(int j=0;j<18;j++)field[i][j]=0;
    field[8][9]=field[9][8]=field[10][9]=field[9][10]=1;
    for(int n=0;n<4;n++){
        int id=bore_arrival_cell(9,9,18,18,used,n,clear);
        assert(id>=0&&clear(id%18,id/18));
        for(int k=0;k<n;k++)assert(used[k]!=id);
        used[n]=(uint16_t)id;
    }
    assert(bore_arrival_cell(9,9,18,18,used,4,clear)<0);
    field[0][0]=1;
    assert(bore_arrival_cell(-100,-100,18,18,used,4,clear)==0);
    puts("Arrival planner: blocked target, four distinct dry cells, exhaustion and bounds PASS");
    return 0;
}
