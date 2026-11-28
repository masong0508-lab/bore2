package com.bore2.game;
public final class LifePacingTest {
 public static void main(String[] args){int minutes=0,remainder=0; for(int i=0;i<100;i++){int[] t=LifeRules.walkClock(remainder,160);minutes+=t[0];remainder=t[1];}if(minutes!=13||remainder!=400)throw new AssertionError("split walking must match whole walk");int[] whole=LifeRules.walkClock(0,16000);if(whole[0]!=minutes||whole[1]!=remainder)throw new AssertionError("walk partition");int[] exact=LifeRules.walkClock(1199,1);if(exact[0]!=1||exact[1]!=0)throw new AssertionError("boundary"); System.out.println("Life pacing PASS");}
}
