package com.bore2.game;
public final class MobileContactsTest {
    static void check(boolean b,String m){if(!b)throw new AssertionError(m);}
    public static void main(String[] args){
        int key=LifeRules.pack(20,30);
        MobileContacts.Resident r=new MobileContacts.Resident(1234,key);
        MobileContacts.Resident same=new MobileContacts.Resident(1234,key);
        check(r.name.equals(same.name)&&r.trait==same.trait&&r.favorite==same.favorite,"stable resident");
        check(!MobileContacts.available(r,true,1,800)&&MobileContacts.available(r,true,1,1140),"weekday resident home schedule");
        MobileContacts b=new MobileContacts();
        check(b.talk(r,0,0,600,true)==-2,"cannot phone stranger");
        check(b.talk(r,0,0,600,false)==3,"first greeting");
        check(b.talk(r,r.favorite,0,610,false)==-1,"anti spam interval");
        check(b.talk(r,r.favorite,0,630,false)==8,"personality-specific topic");
        check(b.talk(r,0,0,660,true)==4,"phone known contact");
        check(b.talk(r,0,0,700,false)==-1,"daily cap");
        check(b.affinity(key)==15,"relationship progress");
        MobileContacts loaded=new MobileContacts();loaded.load(b.save()+";broken;999999999,20,0,0,30");
        check(loaded.affinity(key)==15&&loaded.keys().length==1,"persistence and malformed record guard");
        check(loaded.talk(r,0,1,600,true)==4&&loaded.affinity(key)==19,"next day reset");
        check(loaded.talk(r,0,0,600,false)==-1,"clock cannot go backward");
        check(MobileContacts.isSocialLot(16)&&MobileContacts.isSocialLot(1)&&!MobileContacts.isSocialLot(32),"real lot categories");
        System.out.println("Android stable contacts and relationship progression PASS");
    }
}
