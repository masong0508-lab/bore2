#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "native_life.h"
int main(void) {
    assert(strstr(bore_context_line(BC_WORK,0,50,1,0),"WORK"));
    assert(strstr(bore_context_line(BC_SCHOOL,1,0,1,1),"SCHOOL"));
    assert(strstr(bore_context_line(BC_TRAVEL,0,0,1,2),"WAY"));
    assert(strstr(bore_context_line(BC_EAT,0,0,1,0),"MEAL"));
    assert(strstr(bore_context_line(BC_REST,0,0,1,0),"REST"));
    assert(strcmp(bore_context_line(BC_HOME,0,70,0,0),bore_context_line(BC_HOME,0,0,0,0)));
    assert(strcmp(bore_context_line(BC_OUT,0,10,0,0),bore_context_line(BC_OUT,1,10,0,0)));
    int lines=0, poses=0;
    for(int a=-1;a<=BC_N;a++)for(int tone=0;tone<4;tone++)for(int rel=-100;rel<=100;rel+=25)for(int ask=0;ask<2;ask++)for(unsigned n=0;n<8;n++){
        const char *s=bore_context_line(a,tone,rel,ask,n);
        assert(s && *s && strlen(s)<=39);lines++;
    }
    assert(bore_task_active(BT_EAT,0,0,0,0));
    assert(!bore_task_active(BT_EAT,1,0,0,0));
    assert(!bore_task_active(BT_EAT,0,1,0,0));
    assert(!bore_task_active(BT_EAT,0,0,1,0));
    assert(!bore_task_active(BT_EAT,0,0,0,1));
    assert(!bore_task_active(BT_IDLE,0,0,0,0));
    assert(!bore_task_active(99,0,0,0,0));
    assert(bore_task_phase(0,0)==0 && bore_task_phase(15,0)==0 && bore_task_phase(16,0)==1);
    assert(bore_task_phase(0,1)!=bore_task_phase(0,0));
    for(int task=BT_EAT;task<BT_N;task++)for(unsigned phase=0;phase<2;phase++)for(int side=0;side<2;side++){
        BoreArm arm=bore_task_arm(task,phase,side);
        assert(arm.swing>=-1250&&arm.swing<=0&&arm.elbow>=-1100&&arm.elbow<=0);
        assert(arm.lift>=0&&arm.lift<=90&&arm.reach>=0&&arm.reach<=130);poses++;
    }
    BoreArm a=bore_task_arm(BT_EAT,0,1),b=bore_task_arm(BT_EAT,1,1),left=bore_task_arm(BT_EAT,1,0);
    assert(b.lift>a.lift&&b.reach>a.reach&&b.lift>left.lift);
    assert(bore_task_arm(BT_READ,0,0).swing==bore_task_arm(BT_READ,0,1).swing);
    assert(bore_task_arm(BT_COOK,0,0).reach!=bore_task_arm(BT_COOK,1,0).reach);
    assert(bore_task_arm(BT_IDLE,0,0).lift==0);
    printf("Native life: %d bounded context replies; %d activity-arm samples; motion and pose gates PASS\n",lines,poses);
}
