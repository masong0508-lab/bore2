package com.bore2.game;
public final class ResidentDialogueTest {
    static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    public static void main(String[] args){
        ResidentDialogue atWork=new ResidentDialogue(3,2,35,1,"the office",2,false);
        check(atWork.greeting().contains("exercise")&&atWork.greeting().contains("work"),"remember previous topic and actual activity");
        check(atWork.reply(6).contains("the office"),"day question uses actual workplace");
        check(atWork.reply(2).contains("walking"),"active resident answers relevant favorite topic");
        check(atWork.followUps(2)[0].contains("walk"),"exercise follow-up is contextual");
        check(!atWork.close(2,0).equals(atWork.close(2,1)),"responses differ by selected reaction");
        ResidentDialogue atLunch=new ResidentDialogue(7,3,0,2,"the diner",-1,true);
        check(atLunch.greeting().contains("phone")&&atLunch.reply(6).contains("lunch"),"phone and lunch context");
        check(atLunch.reply(3).contains("cooking")&&atLunch.followUps(3)[1].contains("recipe"),"food conversation stays on topic");
        ResidentDialogue unknown=new ResidentDialogue(0,1,0,-1,null,-1,true);
        check(!unknown.reply(6).contains("work")&&!unknown.reply(6).contains("null"),"unknown schedule not invented");
        for(int t=0;t<16;t++)for(int topic=0;topic<7;topic++){
            ResidentDialogue d=new ResidentDialogue(t,1,20,3,"the park",-1,false);
            check(!d.reply(topic).isEmpty()&&d.followUps(topic).length==3,"complete topic coverage");
            for(int k=0;k<3;k++)check(!d.close(topic,k).isEmpty(),"every reaction closes coherently");
        }
        check(ResidentDialogue.adjustGain(8,0)==10&&ResidentDialogue.adjustGain(8,1)==8&&ResidentDialogue.adjustGain(8,2)==6,"reaction affects relationship gain");
        check(ResidentDialogue.adjustGain(2,2)==1&&ResidentDialogue.adjustGain(-1,0)==-1,"respectful disagreement and cooldown bounds");
        System.out.println("ResidentDialogue: remembered topics, grounded schedules, 112 topic/trait combinations, 336 follow-ups PASS");
    }
}
