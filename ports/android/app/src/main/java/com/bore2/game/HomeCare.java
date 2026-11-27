package com.bore2.game;

/** Clock-driven household condition and one roommate's bounded personal needs. */
final class HomeCare {
    static final int HOME=0,WORK=1,LUNCH=2,LEISURE=3,SLEEP=4,COMMUTE=5,AWAY=-1;
    private static final int NONE=0,CLEANED=1,ATE=2,RESTED=3;
    private int dirt,energy=100,food=100,chores,meals,last;
    interface Schedule {int phase(int weekday,int minute);}
    int dirt(){return dirt;}
    int energy(){return energy;}
    int food(){return food;}
    int chores(){return chores;}
    int meals(){return meals;}
    String lastTask(){switch(last){case CLEANED:return "Cleaned the home";case ATE:return "Had a meal";case RESTED:return "Rested";default:return "No recent household task";}}
    private static int clamp(int x){return Math.max(0,Math.min(100,x));}
    private static int increment(int n){return Math.min(1_000_000,n+1);}
    void resetMember(){energy=food=100;chores=meals=last=0;}
    void clear(){dirt=0;resetMember();}
    boolean clean(){if(dirt==0)return false;dirt=Math.max(0,dirt-25);return true;}
    int restPenalty(){return dirt>=70?5:0;}
    void sharedMeal(){food=clamp(food+40);meals=increment(meals);last=ATE;}
    void advance(int weekday,int minute,int elapsed,boolean leased,boolean roommate,Schedule schedule){
        if(!leased||minute<0||minute>=1440||weekday<0||weekday>=7||elapsed<=0)return;
        // Current native trips are bounded to 3,267 minutes; accept up to one
        // week per action. Tick absolute hour boundaries, not API calls.
        int end=minute+Math.min(elapsed,10080);
        for(int t=(minute/60+1)*60;t<=end;t+=60){
            if(t%180==0)dirt=clamp(dirt+3);
            if(!roommate||schedule==null)continue;
            int phase=schedule.phase((weekday+t/1440)%7,t%1440);
            food=clamp(food-(phase==SLEEP?3:8));
            if(phase==SLEEP){energy=clamp(energy+12);continue;}
            energy=clamp(energy-(phase==WORK||phase==COMMUTE?8:3));
            // The roommate pays for their own food. Never touch player cash or groceries.
            if(food<=55&&(phase==HOME||phase==LUNCH||phase==LEISURE)){food=clamp(food+40);meals=increment(meals);last=ATE;continue;}
            if(phase!=HOME)continue;
            if(energy<=45){energy=clamp(energy+25);last=RESTED;continue;}
            if(dirt>=12){dirt=Math.max(0,dirt-12);energy=clamp(energy-8);chores=increment(chores);last=CLEANED;}
        }
    }
    String save(int seed,int home,int member){return home<0?"":"1,"+seed+","+home+","+member+","+dirt+","+energy+","+food+","+chores+","+meals+","+last;}
    void load(String data,int seed,int home,int member){
        clear();if(home<0||data==null||data.length()>128)return;
        String[] v=data.split(",",-1);if(v.length!=10||!v[0].equals("1"))return;
        try{
            if(Integer.parseInt(v[1])!=seed||Integer.parseInt(v[2])!=home)return;
            int d=Integer.parseInt(v[4]);if(d<0||d>100)return;dirt=d;
            if(member<0||Integer.parseInt(v[3])!=member)return;
            int e=Integer.parseInt(v[5]),f=Integer.parseInt(v[6]),c=Integer.parseInt(v[7]),m=Integer.parseInt(v[8]),l=Integer.parseInt(v[9]);
            if(e<0||e>100||f<0||f>100||c<0||c>1_000_000||m<0||m>1_000_000||l<0||l>RESTED)return;
            energy=e;food=f;chores=c;meals=m;last=l;
        }catch(NumberFormatException ignored){/* Keep a valid home condition, reset invalid personal state. */}
    }
}
