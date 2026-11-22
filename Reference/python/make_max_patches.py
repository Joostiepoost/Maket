# Writes the two Max patches in Max/ (run from the top folder of the repository:
#   python3 Reference/python/make_max_patches.py ). The patches are plain JSON; this script
# only spares writing the boxes and cables by hand. After changing a patch in Max itself,
# the saved patch is the source and this script is out of date for it.
import json
APP={"major":9,"minor":2,"revision":0,"architecture":"x64","modernui":1}
class P:
    def __init__(s): s.boxes=[]; s.lines=[]
    def box(s,id,cls,text,x,y,w,nin,nout,otype,h=22.0,**extra):
        b={"id":id,"maxclass":cls,"numinlets":nin,"numoutlets":nout,"patching_rect":[float(x),float(y),float(w),float(h)]}
        if nout: b["outlettype"]=otype
        if text is not None: b["text"]=text
        b.update(extra); s.boxes.append({"box":b})
    def comment(s,id,text,x,y,w,h=20.0,**extra):
        b={"id":id,"maxclass":"comment","numinlets":1,"numoutlets":0,"patching_rect":[float(x),float(y),float(w),float(h)],"text":text}
        b.update(extra); s.boxes.append({"box":b})
    def line(s,src,so,dst,di): s.lines.append({"patchline":{"source":[src,so],"destination":[dst,di]}})
    def save(s,path,rect):
        ids=[b['box']['id'] for b in s.boxes]; assert len(ids)==len(set(ids))
        info={b['box']['id']:b['box'] for b in s.boxes}
        for l in s.lines:
            a,so=l['patchline']['source']; d,di=l['patchline']['destination']
            assert so<info[a]['numoutlets'],(a,so); assert di<info[d]['numinlets'],(d,di)
        json.dump({"patcher":{"fileversion":1,"appversion":APP,"classnamespace":"box","rect":rect,"boxes":s.boxes,"lines":s.lines,"autosave":0}},open(path,'w'),indent=4)
        print(path,len(s.boxes),'boxes',len(s.lines),'lines')

# ---------------------------------------------------------------- the logging patch
p=P()
p.comment("obj-c1","Maket OSC log",30,20,300,24,fontsize=16.0,fontface=1)
p.comment("obj-c2","Notes when each message arrives in Max: every message gets a number and the time on the computer's clock, in milliseconds since the last reset, and is kept in the coll below. Maket's timing test page resets the log and has it written by itself.",30,48,640,47,linecount=3)

p.box("obj-1","newobj","udpreceive 46002",30,120,110,1,1,[""])
p.comment("obj-c3","the port: the same number as \"Logging patch port\" in Maket's preferences",150,121,440)
p.box("obj-22","newobj","route /maket/log",30,152,110,2,2,["",""])
p.comment("obj-c12","left: Maket's own words for this patch (reset, write <file>); right: everything else, which is logged",150,153,560)

p.box("obj-25","newobj","r maket_log_in",230,184,95,0,1,[""])
p.comment("obj-c13","what is sent to  s maket_log_in  in any patch is logged too (bach.roll's playout)",335,185,470)

p.box("obj-2","newobj","t l b b",121,216,170,1,3,["","bang","bang"])
p.comment("obj-c4","right to left: first the time, then the number, then the message itself",300,217,400)
p.box("obj-3","newobj","cpuclock",272,248,60,1,1,["float"])
p.box("obj-4","newobj","- 0.",272,280,60,2,1,["float"])
p.comment("obj-c5","milliseconds since the last reset",342,281,220)
p.box("obj-5","newobj","int",196,248,40,2,1,["int"])
p.box("obj-6","newobj","+ 1",215,280,40,2,1,["int"])
p.box("obj-7","newobj","pack 0 0.",196,320,95,2,1,[""])
p.box("obj-8","newobj","prepend set",196,352,75,1,1,[""])
p.box("obj-9","newobj","prepend 0",121,392,94,1,1,[""])
p.comment("obj-c6","number, time, message: for example  12 3410.52 noteon Cl 60. 100",225,393,470)
p.box("obj-11","number",None,351,320,60,1,2,["","bang"],parameter_enable=0)
p.comment("obj-c8","the number of the last message",421,321,260)

p.box("obj-10","newobj","coll maket_osc_log",121,470,120,1,4,["","","",""])
p.comment("obj-c7","double-click to see the lines",251,471,200)

p.box("obj-12","toggle",None,421,430,22,1,1,["int"],h=22.0,parameter_enable=0)
p.box("obj-13","newobj","gate",421,470,40,2,1,[""])
p.box("obj-14","newobj","print Maket",421,502,75,1,0,None)
p.comment("obj-c9","switch on to see every message in the Max window as well (not during a timing test)",451,431,330,33,linecount=2)

# Maket's words, and the buttons that do the same by hand
p.box("obj-23","newobj","route reset write",30,560,110,1,3,["","",""])
p.box("obj-24","newobj","prepend write",75,600,85,1,1,[""])
p.comment("obj-c14","write <file>: the coll saves its lines there",170,601,300)
p.box("obj-15","newobj","loadbang",560,560,60,1,1,["bang"])
p.box("obj-16","message","reset",640,560,40,2,1,[""])
p.comment("obj-c10","by hand: empties the log, and starts the clock at 0 and the numbers at 1",690,561,230,33,linecount=2)
p.box("obj-17","newobj","t b b b",560,600,115,1,3,["bang","bang","bang"])
p.box("obj-18","newobj","cpuclock",656,640,60,1,1,["float"])
p.box("obj-19","message","1",608,640,30,2,1,[""])
p.box("obj-20","message","clear",560,640,40,2,1,[""])
p.box("obj-21","message","write",250,600,40,2,1,[""])
p.comment("obj-c11","by hand: saves the log as a text file (Max asks where)",300,601,250,33,linecount=2)

L=p.line
L("obj-1",0,"obj-22",0)
L("obj-22",1,"obj-2",0)       # everything else is logged
L("obj-25",0,"obj-2",0)
L("obj-2",2,"obj-3",0); L("obj-3",0,"obj-4",0); L("obj-4",0,"obj-7",1)
L("obj-2",1,"obj-5",0); L("obj-5",0,"obj-6",0); L("obj-6",0,"obj-5",1); L("obj-5",0,"obj-7",0); L("obj-5",0,"obj-11",0)
L("obj-7",0,"obj-8",0); L("obj-8",0,"obj-9",0)
L("obj-2",0,"obj-9",0)
L("obj-9",0,"obj-10",0); L("obj-9",0,"obj-13",1); L("obj-12",0,"obj-13",0); L("obj-13",0,"obj-14",0)
L("obj-22",0,"obj-23",0)      # Maket's words
L("obj-23",0,"obj-17",0)      # reset
L("obj-23",1,"obj-24",0); L("obj-24",0,"obj-10",0)   # write <file>
L("obj-15",0,"obj-17",0); L("obj-16",0,"obj-17",0)
L("obj-17",2,"obj-18",0); L("obj-18",0,"obj-4",1)
L("obj-17",1,"obj-19",0); L("obj-19",0,"obj-5",1)
L("obj-17",0,"obj-20",0); L("obj-20",0,"obj-10",0)
L("obj-21",0,"obj-10",0)
p.save("Max/Maket OSC log.maxpat",[80.0,100.0,960.0,720.0])

# ---------------------------------------------------------------- the bach.roll reference
q=P()
q.comment("obj-c1","Maket bach reference",30,20,300,24,fontsize=16.0,fontface=1)
q.comment("obj-c2","Plays a passage of Maket's timing test from bach.roll, inside Max, and sends every note it plays to the logging patch (\"Maket OSC log\" must be open). 1. Click read and choose \"timing passage dense.txt\" or \"timing passage long.txt\" in Maket's timing folder. 2. In Maket: Start a bach.roll take. 3. Click play here. 4. When it has played to its end: Finish the bach.roll take in Maket.",30,48,820,62,linecount=4)
q.box("obj-1","message","read",30,125,40,2,1,[""])
q.box("obj-2","message","play",85,125,40,2,1,[""])
q.box("obj-3","message","stop",140,125,40,2,1,[""])
q.box("obj-4","bach.roll",None,30,165,820,6,8,["","","","","","","",""],h=220.0,out="nnnnnntn",bwcompatibility=80901,versionnumber=80901)
q.box("obj-5","newobj","s maket_log_in",731,400,95,1,0,None)
q.comment("obj-c3","the playout outlet (the seventh), set to give text: every note as bach.roll plays it, at the moment it plays it",250,401,470,33,linecount=2)
q.line("obj-1",0,"obj-4",0); q.line("obj-2",0,"obj-4",0); q.line("obj-3",0,"obj-4",0)
q.line("obj-4",6,"obj-5",0)
q.save("Max/Maket bach reference.maxpat",[120.0,140.0,900.0,480.0])
