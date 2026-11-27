package com.bore2.game;

/** A short conversation grounded in the resident's actual current schedule. */
final class ResidentDialogue {
    static final String[] TOPICS={"Say hello","Arts and music","Exercise and outdoors","Food and cooking","Games and technology","Books and stories","How is your day?"};
    private static final String[] SUBJECT={"meeting you","music","exercise","cooking","games","books","your day"};
    private static final String[] INTEREST={"It's nice to have a moment to talk.","I love finding a song that stays with me all day.","I like walking somewhere green when I get a chance.","I enjoy cooking something from scratch.","I like figuring out how a tricky game works.","I love getting lost in a good story."};
    private static final String[] CASUAL={"Hello! How are things?","I don't follow music closely, but I can appreciate a good tune.","A short walk is enough for me most days.","I usually keep cooking simple.","I play a little, but technology isn't really my thing.","I read occasionally, when I have time."};
    private static final String[][] FOLLOW={
        {"I'm glad we ran into each other.","How have things been?","I should keep moving soon."},
        {"I'd enjoy hearing that music.","What do you like about a song?","I prefer a different kind of music."},
        {"A walk sounds lovely.","Where do you like to walk?","I prefer quieter indoor activities."},
        {"Cooking together sounds fun.","What's your favorite recipe?","I prefer a simple meal."},
        {"I'd enjoy trying a game with you.","What makes a good game?","Games aren't really my thing."},
        {"I'd like a book recommendation.","What draws you into a story?","I usually prefer other hobbies."},
        {"I hope your day goes well.","What are you doing after this?","I'll give you some space."}};
    private static final String[] DETAIL={"It's been good to catch up.","A memorable melody matters more to me than a flashy production.","Somewhere with trees and room to breathe is ideal.","I like a recipe that uses a few fresh ingredients well.","Something that rewards curiosity without wasting your time.","Characters who feel like real people make the difference."};
    private final int trait,favorite,affinity,phase,last;
    private final boolean phone;
    private final String place;
    ResidentDialogue(int trait,int favorite,int affinity,int phase,String place,int last,boolean phone){
        this.trait=trait;this.favorite=favorite;this.affinity=affinity;this.phase=phase;this.last=last;this.phone=phone;
        this.place=place==null||place.trim().isEmpty()?"here":place;
    }
    private String activity(){switch(phase){case 0:return "I'm at home.";case 1:return "I'm at work at "+place+".";case 2:return "I'm having lunch at "+place+".";case 3:return "I'm enjoying some free time at "+place+".";case 4:return "I'm trying to get some rest.";case 5:return "I'm on my way somewhere.";default:return "I'm glad you checked in.";}}
    String greeting(){String s=phone?"Thanks for calling. It's nice to hear you on the phone.":affinity>=30?"Good to see you again!":trait==1||trait==8?"Hello. What is it?":"Hi there!";if(last>=1&&last<=5&&affinity>=3)s+=" We talked about "+SUBJECT[last]+" last time.";return s+" "+activity();}
    String reply(int topic){if(topic==6)return activity();if(topic<0||topic>=6)return "What would you like to discuss?";String line=topic==favorite?INTEREST[topic]:CASUAL[topic];if(trait==1||trait==8)line="To be honest, "+Character.toLowerCase(line.charAt(0))+line.substring(1);return line;}
    String[] followUps(int topic){return FOLLOW[topic>=0&&topic<FOLLOW.length?topic:0].clone();}
    String close(int topic,int reaction){if(reaction==1){if(topic==6)return phase==1?"I'll finish my work first, then see what the evening brings.":phase==2?"I'll see how I feel after lunch.":phase==5?"I'll be able to talk more when I arrive.":"I haven't made a definite plan yet.";return DETAIL[topic>=0&&topic<DETAIL.length?topic:0];}if(reaction==2)return "Fair enough. We don't have to agree on everything. Take care!";return topic==6?"Thanks. I appreciate you asking.":"I'm glad we could share that. Talk again soon!";}
    static int adjustGain(int base,int reaction){if(base<=0||reaction<0||reaction>2)return base;return Math.max(1,base+(reaction==0?2:reaction==2?-2:0));}
}
