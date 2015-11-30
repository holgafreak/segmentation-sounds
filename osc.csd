; this is a quick hack csd for getting something out of segm2cs
; (c) -m
; copyright copyleft
<CsoundSynthesizer>
<CsOptions>
-d -odac -+rtaudio=portaudio
</CsOptions>
<CsInstruments>
sr=44100
ksmps=64
nchnls = 2
0dbfs=1

gihandle OSCinit 7778 ;
ga1 init 0
gifnum init 0
gSfile init "osc-initated.wav"
gkvol init 0
gkbasefreq init 0
gkfreqshift init 0
gkpanstart init 0
gkpan init 0
gkmodul init 0
gkquit init 0
turnon 1
;turnon 98 
;for writing to file

instr 1
      imaxinstr = 20
      print gihandle	
      kvol init 0
      kbasefreq init 0
      kfreqshift init 0
      kpanstart init 0
      kpan init 0
      kmodul init 0
      kquit init 0
nxtmsg:
   ;sprintf(msg,"ffffffi %f %f %f %f %f %f %d",vol,basefreq,freqshift,panstart,pan,modul,0);
      kk OSClisten gihandle,"/img2snd/run","ffffffi",kvol,kbasefreq,kfreqshift,kpanstart,kpan,kmodul,kquit
      if(kk==0) goto ex
      kvol = kvol / imaxinstr
      schedkwhen kk,0,imaxinstr,2,0,0.5,kvol,kbasefreq,kfreqshift,kpanstart,kpan,kmodul ; 
      schedkwhen kquit,0,3,3,0,1 ; quit
      kgoto nxtmsg
ex:
      ;prints "waitimg for osc"
endin

instr 3 
      ;turnoff 98
      exitnow
endin

instr 2
      ;asig1,asig2 inch 1,2
      kvol= p4
      kstr = p7
      kend = p8
      kmodul = p9
      kfrq = p5
      kfrqsh = p6
      ;kt line istr,0.5,iend
      kdeclick linseg 0,0.1,1,0.3,1,0.1,0
      kdeclick *= 10 ; some scaling here and there
      amod oscil kdeclick*kvol,kmodul,1
      a1 oscil kdeclick*kvol,kfrq,1
      a2 oscil kdeclick*kvol,kfrq+kfrqsh,1
      a1 = a1*amod; simple ring modulator
      a2= a2*amod 
      out kstr*a1,kend*a2
endin
instr 98
      prints    "write\n"
      a1,a2 monitor
      fout gSfile,14,a1,a2
endin

instr 99
      
      exitnow
endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1 0.33 0.12 0.8
f0 3600
e
</CsScore>
</CsoundSynthesizer>
