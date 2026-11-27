package com.bore2.game;
import java.util.Arrays;
import java.util.HashSet;
public final class AvatarStyleTest {
    static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    public static void main(String[] args){
        AvatarStyle base=AvatarStyle.defaults();
        check(base.choice(0)==2&&base.choice(1)==1&&base.choice(2)==0&&base.choice(3)==0&&base.choice(4)==0,"readable default appearance");
        for(int k=0;k<5;k++)for(int c=0;c<AvatarStyle.options(k).length;c++){
            AvatarStyle next=base.with(k,c),saved=AvatarStyle.decode(next.encode());
            check(saved.choice(k)==c,"save choice round trip");
            for(int j=0;j<5;j++)if(j!=k)check(saved.choice(j)==base.choice(j),"unrelated choice preserved");
        }
        check(base.with(0,999).encode()==base.encode()&&base.with(-1,0).encode()==base.encode(),"invalid edit ignored");
        check(AvatarStyle.decode(-1).encode()==base.encode(),"invalid save falls back");
        check(AvatarStyle.cleanName("  Ada\n  Grove  ").equals("Ada Grove"),"normalize whitespace");
        check(AvatarStyle.cleanName("\u0000\u0001").equals("Bore"),"empty name fallback");
        check(AvatarStyle.cleanName("abcdefghijklmnopqrstuvwxyz").length()==20,"name bound");
        HashSet<Integer> styles=new HashSet<>();
        for(int key=0;key<100;key++){
            int a=AvatarStyle.resident(1234,key).encode(),b=AvatarStyle.resident(1234,key).encode();
            check(a==b,"same resident appearance");styles.add(a);
        }
        check(styles.size()>50,"residents not clones");
        System.out.println("Avatar style: stable appearance, validated persistence, name handling PASS");
    }
}
