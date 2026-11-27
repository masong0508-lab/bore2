package com.bore2.game;

import java.util.LinkedHashMap;
import java.util.Map;

/** Mobile-only resident identities anchored to generated residential blocks. */
final class MobileContacts {
    static final String[] TRAITS={"Cheerful","Grumpy","Lazy","Active","Creative","Geek","Romantic","Foodie","Loner","Outgoing","Neat","Messy","Goofball","Bookworm","Music lover","Family-oriented"};
    static final String[] TOPICS=ResidentDialogue.TOPICS.clone();
    private static final String[] FIRST={"Ada","Alf","Arlo","Bea","Ben","Cleo","Dale","Ella","Finn","Hazel","Ivy","Jax","Juno","Kai","Leo","Luna","Mae","Milo","Nell","Nova","Otis","Pearl","Quinn","Rosa","Ruby","Sid","Sky","Sunny","Tess","Toby","Wes","Zora"};
    private static final String[] LAST={"Ash","Baker","Brook","Cedar","Clarke","Dale","Ellis","Frost","Grove","Hall","Hayes","Hill","Lane","Marsh","Moss","Reed","Rowan","Sage","Shaw","Stone","Vale","West","Woods","Young"};
    private static final int[] FAVORITES={1,5,4,2,1,4,1,3,5,3,3,4,4,5,1,3};
    private static int mix(int v){v^=v>>>16;v*=0x7feb352d;v^=v>>>15;v*=0x846ca68b;return v^(v>>>16);}
    static final class Resident {
        final int key,trait,favorite;final String name;
        Resident(int seed,int key){this.key=key;int h=mix(seed^key);trait=h&15;favorite=FAVORITES[trait];name=FIRST[(h>>>4)&31]+" "+LAST[(h>>>9)%LAST.length];}
    }
    static boolean isSocialLot(int lot){return LifeRules.isHome(lot)||LifeRules.isVenue(lot)||lot==7||lot==17||lot==18||lot==22||lot==25;}
    static boolean available(Resident r,boolean atResidence,int weekday,int minute){
        if(minute<420||minute>=1320)return false;
        boolean free=weekday>=5||minute<480||minute>=1020;
        if(atResidence)return free;
        return minute>=540&&free;
    }
    private static final class Entry {int affinity,day=-1,count,minute,lastTopic=-1;}
    private final LinkedHashMap<Integer,Entry> entries=new LinkedHashMap<>();
    int affinity(int key){Entry e=entries.get(key);return e==null?0:e.affinity;}
    int lastTopic(int key){Entry e=entries.get(key);return e==null?-1:e.lastTopic;}
    int[] keys(){int[] a=new int[entries.size()];int i=0;for(int key:entries.keySet())a[i++]=key;return a;}
    // Positive = affinity gain. -1 cooldown, -2 unknown call, -3 book full/invalid.
    int talk(Resident r,int topic,int day,int minute,boolean phone){return converse(r,topic,day,minute,phone,-1);}
    int converse(Resident r,int topic,int day,int minute,boolean phone,int reaction){
        if(r==null||!LifeRules.inside(LifeRules.x(r.key),LifeRules.y(r.key))||topic<0||topic>=TOPICS.length||day<0||minute<0||minute>=1440)return -3;
        Entry e=entries.get(r.key);
        if(phone&&(e==null||e.affinity<3))return -2;
        if(e==null){if(entries.size()>=64)return -3;e=new Entry();entries.put(r.key,e);}
        if(e.day>day||(e.day==day&&(e.count>=3||minute-e.minute<30)))return -1;
        int gain=ResidentDialogue.adjustGain(phone?4:topic==0||topic==6?3:topic==r.favorite?8:2,reaction);
        if(e.day!=day)e.count=0;
        e.day=day;e.minute=minute;e.count++;e.lastTopic=topic;e.affinity=Math.min(100,e.affinity+gain);
        return gain;
    }
    String save(){StringBuilder s=new StringBuilder();for(Map.Entry<Integer,Entry> item:entries.entrySet()){
        Entry e=item.getValue();if(s.length()>0)s.append(';');s.append(item.getKey()).append(',').append(e.affinity).append(',').append(e.day).append(',').append(e.count).append(',').append(e.minute).append(',').append(e.lastTopic);
    }return s.toString();}
    void load(String data){entries.clear();if(data==null||data.length()>8192)return;
        for(String line:data.split(";")){if(entries.size()>=64)break;String[] v=line.split(",",-1);if(v.length!=5&&v.length!=6)continue;
            try{int key=Integer.parseInt(v[0]),aff=Integer.parseInt(v[1]),day=Integer.parseInt(v[2]),count=Integer.parseInt(v[3]),min=Integer.parseInt(v[4]);
                if(!LifeRules.inside(LifeRules.x(key),LifeRules.y(key))||aff<1||aff>100||day<0||count<1||count>3||min<0||min>=1440)continue;
                Entry e=new Entry();e.affinity=aff;e.day=day;e.count=count;e.minute=min;
                if(v.length==6){int topic=Integer.parseInt(v[5]);if(topic>=0&&topic<TOPICS.length)e.lastTopic=topic;}
                entries.put(key,e);
            }catch(NumberFormatException ignored){}
        }
    }
}
