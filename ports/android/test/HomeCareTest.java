package com.bore2.game;
public final class HomeCareTest {
    static void check(boolean b,String why){if(!b)throw new AssertionError(why);}
    static void same(HomeCare a,HomeCare b){check(a.save(1234,500,501).equals(b.save(1234,500,501)),"time chunking cannot change simulation");}
    public static void main(String[] args){
        HomeCare a=new HomeCare(),b=new HomeCare();
        a.advance(0,0,720,true,false,(d,m)->HomeCare.AWAY);
        for(int t=0;t<720;t+=15)b.advance(0,t,15,true,false,(d,m)->HomeCare.AWAY);
        check(a.dirt()==12,"one dirt increment per three-hour boundary");same(a,b);
        check(a.chores()==0,"no phantom roommate chores");
        a.advance(0,720,720,true,true,(d,m)->HomeCare.HOME);
        check(a.chores()>0&&a.dirt()<12,"roommate cleans when home");
        check(a.food()>=0&&a.food()<=100&&a.energy()>=0&&a.energy()<=100,"needs stay bounded");
        check(a.meals()>0,"roommate feeds self");
        HomeCare shared=new HomeCare();shared.advance(0,0,360,true,true,(d,m)->HomeCare.WORK);
        check(shared.food()==52,"work uses food reserves");
        shared.sharedMeal();check(shared.food()==92&&shared.meals()==1,"actual shared meal feeds roommate");
        HomeCare roll=new HomeCare();roll.advance(6,1380,60,true,true,(d,m)->{check(d==0&&m==0,"week rollover");return HomeCare.SLEEP;});
        HomeCare sleeping=new HomeCare();sleeping.advance(0,0,480,true,true,(d,m)->HomeCare.WORK);
        int tired=sleeping.energy();sleeping.advance(0,480,480,true,true,(d,m)->HomeCare.SLEEP);
        check(tired<100&&sleeping.energy()>tired,"work drains and sleep restores energy");
        check(sleeping.chores()==0,"no chores at work or asleep");
        HomeCare away=new HomeCare();away.advance(0,0,1440,true,true,(d,m)->HomeCare.AWAY);check(away.chores()==0,"no remote cleaning");
        HomeCare longRun=new HomeCare(),chunked=new HomeCare();
        HomeCare.Schedule schedule=(d,m)->m<420||m>=1320?HomeCare.SLEEP:m>=480&&m<1020?HomeCare.WORK:HomeCare.HOME;
        longRun.advance(2,1175,3000,true,true,schedule);
        for(int i=0;i<3000;i+=5){int time=1175+i;chunked.advance(2+time/1440,time%1440,5,true,true,schedule);}same(longRun,chunked);
        HomeCare dirty=new HomeCare();dirty.advance(0,0,10080,true,false,schedule);check(dirty.dirt()==100,"dirt bounded");
        check(dirty.restPenalty()==5,"dirty home has a modest real rest penalty");
        check(dirty.clean()&&dirty.dirt()==75,"manual cleaning changes condition");
        dirty.clean();check(dirty.restPenalty()==0,"cleaning restores comfort");
        HomeCare saved=new HomeCare();saved.load(longRun.save(1234,500,501),1234,500,501);same(saved,longRun);
        saved.load(longRun.save(1234,500,501),1234,500,502);check(saved.dirt()==longRun.dirt()&&saved.chores()==0&&saved.energy()==100,"new roommate inherits no previous needs or stats");
        saved.load(longRun.save(1234,500,501),99,500,501);check(saved.dirt()==0,"wrong world ignored");
        saved.load("1,1234,500,501,999,100,100,0,0,0",1234,500,501);check(saved.dirt()==0,"corrupt save rejected");
        saved.advance(0,0,10080,false,true,schedule);check(saved.dirt()==0&&saved.chores()==0,"no leased home no simulation");
        System.out.println("HomeCare: clock chunk invariance, chores only at home, needs, comfort, bounds and save identity PASS");
    }
}
