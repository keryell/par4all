*DECK TWLDRV
      SUBROUTINE TWLDRV(ISCF,DM,DN,ICRIT,USESYM,NSYMOP,JTRANS,NEQATM,
     $    NATOMS,C,VEE,FXYZ,IDUMP,FROZEN,NONCOR,NONC1,FHAM,AJ,AK,
     $    CMO,NBASIS,INDDO,ATMCHG,FIRSTS,ICHECK)
      IMPLICIT REAL*8(A-H,O-Z)

C	BUG FIX.  (ANSI X3.9-1978, SECTION 15.9.3.2, LINE 25)
	DIMENSION AFFQ(6)
	EQUIVALENCE (AFFQ(1) , FQ0)

C
C     EVALUATION OF THE TWO ELECTRON INTEGRAL CONTRIBUTION TO THE
C     FORCES.
C
      INTEGER SHELLA,SHELLN,SHELLT,AOS,AON,SHELLC
      LOGICAL USESYM,IJSAME,KLSAME,IKSMJL,FLAG,FROZEN(NATOMS),
     $        FI,FIJ,FIJK,FIRSTS(1)
C***  LOGICAL CHKSHL
      DIMENSION FHAM(1), AJ(1), AK(1)
      DIMENSION CMO(1), ATMCHG(1)
      COMMON/IO/IN,IOUT,IPUNCH
      COMMON /FFQ/ FQ0,FQ1,FQ2,FQ3,FQ4,FQ5
      COMMON/B/EXX(1200),C1(1200),C2(1200),C3(1200),
     $         X(400),Y(400),Z(400),JAN(400),SHELLA(400),SHELLN(400),
     $         SHELLT(400),SHELLC(400),AOS(400),AON(400),NSHELL,MAXTYP
      DIMENSION C4(400),SHLADF(400)
      EQUIVALENCE(C4(1),C3(401)),(SHLADF(1),C3(801))
      COMMON /FP4/ QA,QA1,QA2,A12I,A34I,A1234I
      COMMON /FP4/ A1,A2,A3,A4,A12,A34,A1234,PQX,PQY,PQZ,
     *   PQXX,PQYY,PQZZ,PQXY,PQXZ,PQYZ,
     *V0000,V0010,V0020,V0030,V0100,V0200,V0300,
     *V0110,V0120,V0130,V0210,V0220,V0230,V0310,V0320,V0330,
     *V1010,V1020,V1030,V2010,V2020,V2030,V3010,V3020,V3030,
     *V1000,V2000,V3000,V1100,V2100,V3100,V1200,V2200,V3200,
     *V1300,V2300,V3300,V1110,V2110,V3110,V1210,V2210,V3210,
     *V1310,V2310,V3310,V1120,V2120,V3120,V1220,V2220,V3220,
     *V1320,V2320,V3320,V1130,V2130,V3130,V1230,V2230,V3230,
     *V1330,V2330,V3330
      COMMON /FP4/C1110,C2110,C3110,C1210,C2210,C3210,
     *C1320,C2320,C3320,C1130,C2130,C3130,C1230,C2230,C3230,
     *C1310,C2310,C3310,C1120,C2120,C3120,C1220,C2220,C3220,
     *C1330,C2330,C3330,OPXO,OPYO,OPZO,OPOX,OPOY,OPOZ,OPXOX,OPYOY,OPZOZ,
     *OPXX,OPXY,OPXZ,OPYX,OPYY,OPYZ,OPZX,OPZY,OPZZ,OQXO,OQYO,OQZO,
     *OQOX,OQOY,OQOZ,OQXOX,OQYOY,OQZOZ,OQXX,OQXY,OQXZ,OQYX,OQYY,OQYZ,
     *OQZX,OQZY,OQZZ,S1,S2,S3,S4,S12,S34
      COMMON /FP4/ E,
     *GOOOO,GOOXO,GOOYO,GOOZO,GXOOO,GXOXO,GXOYO,GXOZO,GXXOO,GXXXO,GXXYO,
     *GXXZO,GXYOO,GXYZO,GXZOO,GYOOO,GYOYO,GYOZO,GYYOO,GYYXO,GYYYO,
     *GYYZO,GYZOO,GZOOO,GZOZO,GZZOO,GZZXO,GZZYO,GZZZO,
     *VE00,VE11,VE12,VE13,VE14,VE21,VE22,VE23,VE24,VE31,VE32,VE33,VE34,
     *CSSSP,CSSPP,CSPSP,CPSSP,CSPPP,CPSPP,CPPSP,CPPPP
      COMMON /TABLE/ TBAA(400),TBBA(400),TBCA(400),
     *               TBAB(400),TBBB(400),TBCB(400),
     *               TBAC(400),TBBC(400),TBCC(400),
     *               TBAD(400),TBBD(400),TBCD(400),
     *               TBAE(400),TBBE(400),TBCE(400),
     *               TBAF(400),TBBF(400),TBCF(400)
      DIMENSION DM(1),DN(1),JTRANS(3,8),NEQATM(1),C(1),FXYZ(1),E(256)
      DIMENSION XE34(100),QX(100),QY(100),QZ(100),TA34I(100),TS3(100),
     $          TS4(100),TS34(100),CSMCD(100),ISYMOP(8), IPRIO(8)
C***  DATA IRWTAB/503/
      DATA THREE,P25,TWO,H,ZERO,ONE5,TWENTY,TWO5,THREE5,FOUR5,
     $ SXTN,ONE,FOUR,TEN,F60
     $ /3.D0,0.25D0,2.D0,0.5D0,0.D0,1.5D0,20.D0,2.5D0,3.5D0,
     $  4.5D0,16.D0,1.D0,4.D0,10.D0,60.D0/
      DATA EXPCUT/100.D0/,F19/19.D0/,F100/100.D0/
 2005 FORMAT(1X,4I2,12F10.6)
 2006 FORMAT(6H SYMOP,I3,18X,I2,38X,I2,38X,I2)
 2007 FORMAT(' TWLDRV:  FMTGEN WAS CALLED ',I9,' TIMES.')
C
C     READ IN F(M,T) INTERPOLATION TABLE FROM THE RW-FILES.
C***      CALL TRAKIO(6HTWLDRV,1)
C***      CALL TREAD(IRWTAB,TBAA(1),1200,6,1200,6,0)
      CALL GAMGEN
C
C     COMPUTE PI RELATED CONSTANTS.
      PI=FOUR*DATAN(ONE)
      PITO52=TWO*(PI**TWO5)
      PIDIV4=P25*PI
      CALL FMTSET(0,0,0)
      IFMT=0
C
C     CLEAR OUT ACCUMELATOR VEE.
      VEE=ZERO
C
C     SET UP CRITICAL CONSTANTS.
      ICRITP=ICRIT+1
      GO TO(11,12,13),ICRITP
C.....  STANDARD CUTOFFS.
   11 VTOL1=TEN**(-20)
      VTOL2=TEN**(-20)
      VTOLS=TEN**(-30)
      FMT1=F19
      FMT2=F60
      GO TO 14
C.....  TEST CUTOFFS.
   12 VTOL1=TEN**(-30)
      VTOL2=TEN**(-30)
      VTOLS=TEN**(-30)
      FMT1=ZERO
      FMT2=F100
      GO TO 14
C.....  'BERNY' CUTOFFS.
   13 VTOL1=TEN**(-11)
      VTOL2=TEN**(-9)
      VTOLS=TEN**(-14)
      FMT1=SXTN
      FMT2=ZERO
   14 CONTINUE
C
C        BEGIN LOOPS OVER THE SHELLS
C
C***      CALL SETPN(NSHELL)
      DO 1001 ISHELL=1,NSHELL
C***      CALL DECRPN
      IF(SHELLT(ISHELL).GT.1) GO TO 1001
      FI=FROZEN(JAN(ISHELL))
      DO 1002 JSHELL=1,ISHELL
      IF(SHELLT(JSHELL).GT.1) GO TO 1002
      FIJ=FI.AND.FROZEN(JAN(JSHELL))
      DO 1003 KSHELL=1,ISHELL
      IF(SHELLT(KSHELL).GT.1) GO TO 1003
      FIJK=FIJ.AND.FROZEN(JAN(KSHELL))
      DO 1004 LSHELL=1,KSHELL
      IF(SHELLT(LSHELL).GT.1) GO TO 1004
      IF(FIJK.AND.FROZEN(JAN(LSHELL))) GOTO 1004
C***      IF(.NOT.CHKSHL(ISHELL,JSHELL,KSHELL,LSHELL,JAN,ICHECK)) GOTO 1004
      IF(KSHELL.EQ.ISHELL.AND.LSHELL.GT.JSHELL) GO TO 1002
      AX1=H
      IF(ISHELL.NE.JSHELL) AX1=AX1+AX1
      IF(KSHELL.NE.LSHELL) AX1=AX1+AX1
      IF(ISHELL.NE.KSHELL.OR.JSHELL.NE.LSHELL) AX1=AX1+AX1
C***      WRITE(IOUT,*) ISHELL,JSHELL,KSHELL,LSHELL,AX1
C
C        PUT SHELLS INTO STANDARD ORDER
C
      INEW=ISHELL
      JNEW=JSHELL
      KNEW=KSHELL
      LNEW=LSHELL
      LA=SHELLT(INEW)
      LB=SHELLT(JNEW)
      LC=SHELLT(KNEW)
      LD=SHELLT(LNEW)
      IF(LA-LB) 1011,1012,1012
 1011 INEW=JSHELL
      JNEW=ISHELL
 1012 IF(LC-LD) 1013,1014,1014
 1013 KNEW=LSHELL
      LNEW=KSHELL
 1014 IF(LA+LB-LC-LD) 1015,1016,1016
 1015 ID=INEW
      INEW=KNEW
      KNEW=ID
      ID=JNEW
      JNEW=LNEW
      LNEW=ID
 1016 LA=3*SHELLT(INEW)+1
      LB=3*SHELLT(JNEW)+1
      LC=3*SHELLT(KNEW)+1
      LD=3*SHELLT(LNEW)+1
C
C        OBTAIN INFORMATION ABOUT THE SHELLS
C
      IJSAME=INEW.EQ.JNEW
      KLSAME=KNEW.EQ.LNEW
      IKSMJL=(INEW.EQ.KNEW).AND.(JNEW.EQ.LNEW)
      JTYPE=(LA+LB+LC+LC+LD-2)/3
C###TEMP ST.
C***      IF(JTYPE.GT.6)CALL LNK1E
      IFQMAX=SHELLT(INEW)+SHELLT(JNEW)+SHELLT(KNEW)+SHELLT(LNEW)+1
      IAT=JAN(INEW)
      JAT=JAN(JNEW)
      KAT=JAN(KNEW)
      LAT=JAN(LNEW)
      IF((IAT.EQ.JAT).AND.(IAT.EQ.KAT).AND.(IAT.EQ.LAT)) GO TO 1004
      IF(.NOT.USESYM) GOTO 60
C
C     LOOP OVER SYMMETRY OPERATIONS.  OF THE EQUIVALENT SETS OF SHELLS
C     PRODUCED, ONLY THE SET WITH THE HIGHEST "PRIORITY" WILL BE USED.
C
      NUMOP = 1
      NP1 = NPRIO(IAT,JAT,KAT,LAT,NSHELL)
      ISYMOP(1) = 1
      IPRIO(1) = NP1
      JOPIND=0
      DO 30 JOP=2,NSYMOP
      JOPIND=JOPIND+NATOMS
      IAT1 = NEQATM(IAT+JOPIND)
      JAT1 = NEQATM(JAT+JOPIND)
      KAT1 = NEQATM(KAT+JOPIND)
      LAT1 = NEQATM(LAT+JOPIND)
      NP2 = NPRIO(IAT1,JAT1,KAT1,LAT1,NSHELL)
      IF(NP2 .GT. NP1) GOTO 1004
      NUMOP = NUMOP + 1
      ISYMOP(NUMOP) = JOP
      IPRIO(NUMOP) = NP2
   30 CONTINUE
C
C     REMOVE REDUNDANT OPERATIONS.
C
      MOP = NUMOP - 1
      DO 50 JROP=1,MOP
      JOP = NUMOP - JROP + 1
      JOPM1 = JOP - 1
      DO 40 KOP=1,JOPM1
      IF(IPRIO(KOP) .NE. IPRIO(JOP)) GOTO 40
      ISYMOP(JOP) = 0
      GOTO 50
   40 CONTINUE
   50 CONTINUE
C
   60 CONTINUE
      IATX=3*(IAT-1)+1
      JATX=3*(JAT-1)+1
      KATX=3*(KAT-1)+1
      LATX=3*(LAT-1)+1
      IATY=IATX+1
      JATY=JATX+1
      KATY=KATX+1
      LATY=LATX+1
      IATZ=IATY+1
      JATZ=JATY+1
      KATZ=KATY+1
      LATZ=LATY+1
      AX=X(INEW)
      BX=X(JNEW)
      CX=X(KNEW)
      DX=X(LNEW)
      AY=Y(INEW)
      BY=Y(JNEW)
      CY=Y(KNEW)
      DY=Y(LNEW)
      AZ=Z(INEW)
      BZ=Z(JNEW)
      CZ=Z(KNEW)
      DZ=Z(LNEW)
      ABX=AX-BX
      ABY=AY-BY
      ABZ=AZ-BZ
      CDX=CX-DX
      CDY=CY-DY
      CDZ=CZ-DZ
      R34=CDX**2+CDY**2+CDZ**2
      R12=ABX**2+ABY**2+ABZ**2
      ISTART=SHELLA(INEW)
      JSTART=SHELLA(JNEW)
      KSTART=SHELLA(KNEW)
      LSTART=SHELLA(LNEW)
      IEND=ISTART+SHELLN(INEW)-1
      JEND=JSTART+SHELLN(JNEW)-1
      KEND=KSTART+SHELLN(KNEW)-1
      LEND=LSTART+SHELLN(LNEW)-1
C
C     FILL E WITH THE REQUIRED DENSITY MATRIX CONTRIBUTIONS,
C     AND AT THE SAME TIME, GET DMAX.
      CALL EFILL(INEW,JNEW,KNEW,LNEW,LA,LB,LC,LD,AX1,ISCF,DM,DN,E,DMAX,
     $    NBASIS,NONCOR,NONC1,FHAM,AJ,AK,CMO,INDDO,ATMCHG,FIRSTS)

C
C     REJECT CURRENT SHELL CASE IF DMAX IS .LT. VTOL.
      IF(DMAX.LT.VTOL1) GO TO 1004
C
C     HUNT OUT LARGEST DENSITY*CONTRACTION, AND REJECT IF POSSIBLE.
      E34MAX=ZERO
      KZERO=-10
      DO 2 K=KSTART,KEND
      KZERO=KZERO+10
      KL=KZERO
      A3=EXX(K)
      CSUMC=DABS(C1(K))+DABS(C2(K))
      LND=LEND
      IF(KLSAME) LND=K
      DO 2 L=LSTART,LND
      A4=EXX(L)
      KL=KL+1
      A34=A3+A4
      A34I=ONE/A34
      S3=A3*A34I
      S4=A4*A34I
      S34=A3*S4
      QX(KL)=S3*CX+S4*DX
      QY(KL)=S3*CY+S4*DY
      QZ(KL)=S3*CZ+S4*DZ
      TA34I(KL)=A34I
      TS3(KL)=S3
      TS4(KL)=S4
      TS34(KL)=S34
      EXPARG=R34*S34
      E34=ZERO
      IF(EXPARG.LT.EXPCUT)E34=DEXP(-EXPARG)*A34I
      IF(KLSAME.AND.(K.NE.L)) E34=E34+E34
      XE34(KL)=E34
      E34=E34*CSUMC*(DABS(C1(L))+DABS(C2(L)))
      IF(E34.GT.E34MAX)E34MAX=E34
      CSMCD(KL)=E34**2
    2 CONTINUE
      IF(DMAX*E34MAX.LT.VTOL1) GO TO 1004
      FXI=ZERO
      FXJ=ZERO
      FXK=ZERO
      FXL=ZERO
      FYI=ZERO
      FYJ=ZERO
      FYK=ZERO
      FYL=ZERO
      FZI=ZERO
      FZJ=ZERO
      FZK=ZERO
      FZL=ZERO
      VE11=ZERO
      VE12=ZERO
      VE13=ZERO
      VE14=ZERO
      VE21=ZERO
      VE22=ZERO
      VE23=ZERO
      VE24=ZERO
      VE31=ZERO
      VE32=ZERO
      VE33=ZERO
      VE34=ZERO
C
C        LOOP OVER THE UNCONTRACTED GAUSSIANS WITHIN THE SHELLS
C
      IZERO=-10
C
C..... GAUSSIANS AT CENTER A.
      DO 2001 I=ISTART,IEND
      IZERO=IZERO+10
      IJ=IZERO
      A1=EXX(I)
      CSA=C1(I)
      CPA=C2(I)
      CSUMA=(DABS(CSA)+DABS(CPA))*DMAX
      JND=JEND
      IF(IJSAME) JND=I
C
C..... GAUSSIANS AT CENTER B.
      DO 2002 J=JSTART,JND
      IJ=IJ+1
      A2=EXX(J)
      A12=A1+A2
      A12I=ONE/A12
      S1=A1*A12I
      S2=A2*A12I
      S12=A1*S2
      EXPARG=R12*S12
      E12=ZERO
      IF(EXPARG.LT.EXPCUT)E12=DEXP(-EXPARG)*PITO52*A12I
      IF(IJSAME.AND.(I.NE.J)) E12=E12+E12
      CSB=C1(J)*E12
      CPB=C2(J)*E12
      CSMAB=CSUMA*(DABS(CSB)+DABS(CPB))
      IF(CSMAB*E34MAX.LT.VTOL2) GO TO 2002
      CSMAB=CSMAB**2
      CSS=CSA*CSB
C     INITIALIZE C.. VARIABLES IN CASE LATER IF-CHECKS CIRCUMVENT
C     THEIR CALCULATION.
      CPS=ZERO
      CSP=ZERO
      CPP=ZERO
      PX=S1*AX+S2*BX
      PY=S1*AY+S2*BY
      PZ=S1*AZ+S2*BZ
      IF(LA.EQ.1) GO TO 51
      CPS=CPA*CSB
      OPXO=-S2*ABX
      OPYO=-S2*ABY
      OPZO=-S2*ABZ
      IF(LB.EQ.1) GO TO 51
      CSP=CSA*CPB
      CPP=CPA*CPB
      OPOX=S1*ABX
      OPOY=S1*ABY
      OPOZ=S1*ABZ
      OPXOX=OPXO+OPOX
      OPYOY=OPYO+OPOY
      OPZOZ=OPZO+OPOZ
      HA12I=H*A12I
      OPXX=OPXO*OPOX+HA12I
      OPYY=OPYO*OPOY+HA12I
      OPZZ=OPZO*OPOZ+HA12I
      OPXY=OPXO*OPOY
      OPYX=OPXY
      OPXZ=OPXO*OPOZ
      OPZX=OPXZ
      OPYZ=OPYO*OPOZ
      OPZY=OPYZ
   51 CONTINUE
      VE00S=ZERO
      VE11S=ZERO
      VE12S=ZERO
      VE21S=ZERO
      VE22S=ZERO
      VE31S=ZERO
      VE32S=ZERO
      DVEXS=ZERO
      DVEYS=ZERO
      DVEZS=ZERO
      KND=KEND
      IF(IKSMJL) KND=I
      KZERO=-10
C
C..... GAUSSIANS AT CENTER C.
      DO 2003 K=KSTART,KND
      KZERO=KZERO+10
      KL=KZERO
      A3=EXX(K)
      CSC=C1(K)
      CPC=C2(K)
      CSSS=CSS*CSC
      CSSP=CSS*CPC
      CPSS=CPS*CSC
      CPSP=CPS*CPC
      CSPS=CSP*CSC
      CSPP=CSP*CPC
      CPPS=CPP*CSC
      CPPP=CPP*CPC
      LND=LEND
      IF(KLSAME) LND=K
      IF(IKSMJL.AND.(I.EQ.K)) LND=J
C
C..... GAUSSIANS AT CENTER D.
      DO 2004 L=LSTART,LND
      KL=KL+1
      A34=A3+EXX(L)
      A1234=A12+A34
      A1234I=ONE/A1234
      VTEST=CSMAB*CSMCD(KL)*A1234I
      IF(VTEST.LT.VTOLS) GO TO 2004
      QA2=A34*A1234I
      QA=A12*QA2
      PQX=PX-QX(KL)
      PQY=PY-QY(KL)
      PQZ=PZ-QZ(KL)
      PQXX=PQX*PQX
      PQYY=PQY*PQY
      PQZZ=PQZ*PQZ
      T=QA*(PQXX+PQYY+PQZZ)
      A34I=TA34I(KL)
      S3=TS3(KL)
      S4=TS4(KL)
      S34=TS34(KL)
      E34=XE34(KL)*DSQRT(A1234I)
      IF(IKSMJL.AND.(IJ.NE.KL)) E34=E34+E34
      CSD=C1(L)*E34
      EOOOO=E(1)*CSSS*CSD
      IF(T.LE.FMT1)GO TO 100
      IF(T.LE.FMT2)GO TO 120
      TI=ONE/T
      IF(VTEST*TI.LT.VTOLS) GO TO 2004
      FQ0=DSQRT(PIDIV4*TI)
      FQ1=H*FQ0*TI
      IF(JTYPE.EQ.1) GO TO 3003
      FQ2=ONE5*FQ1*TI
      IF(JTYPE.EQ.2) GO TO 3004
      FQ3=TWO5*FQ2*TI
      FQ4=THREE5*FQ3*TI
      FQ5=FOUR5*FQ4*TI
      GO TO 110
  100 CONTINUE
      QQ=T*TWENTY
      THETA=QQ-AINT(QQ)
      N=QQ-THETA
      THETA2=THETA*(THETA-ONE)
      THETA3=THETA2*(THETA-TWO)
      THETA4=THETA2*(THETA+ONE)
      GO TO (101,102,103,104,105), IFQMAX
  105 FQ5=TBAF(N+1)+THETA*TBBF(N+1)-THETA3*TBCF(N+1)+THETA4*TBCF(N+2)
  104 FQ4=TBAE(N+1)+THETA*TBBE(N+1)-THETA3*TBCE(N+1)+THETA4*TBCE(N+2)
  103 FQ3=TBAD(N+1)+THETA*TBBD(N+1)-THETA3*TBCD(N+1)+THETA4*TBCD(N+2)
  102 FQ2=TBAC(N+1)+THETA*TBBC(N+1)-THETA3*TBCC(N+1)+THETA4*TBCC(N+2)
  101 FQ1=TBAB(N+1)+THETA*TBBB(N+1)-THETA3*TBCB(N+1)+THETA4*TBCB(N+2)
      FQ0=TBAA(N+1)+THETA*TBBA(N+1)-THETA3*TBCA(N+1)+THETA4*TBCA(N+2)
      GO TO 110
  120 IFMT=IFMT+1
      CALL FMTGEN(AFFQ,T,IFQMAX+1,ICK)
  110 CONTINUE
      IF(JTYPE.EQ.1) GO TO 3003
      IF(JTYPE.EQ.2) GO TO 3004
      PQXY=PQX*PQY
      PQYZ=PQY*PQZ
      PQXZ=PQX*PQZ
      QA1=A12*A1234I
      CPD=C2(L)*E34
      IF(LC.EQ.1) GO TO 52
      CSSPS=CSSP*CSD
      CPSPS=CPSP*CSD
      OQXO=-S4*CDX
      OQYO=-S4*CDY
      OQZO=-S4*CDZ
      IF(LD.EQ.1) GO TO 52
      CSSSP=CSSS*CPD
      CSSPP=CSSP*CPD
      CSPSP=CSPS*CPD
      CPSSP=CPSS*CPD
      CSPPP=CSPP*CPD
      CPSPP=CPSP*CPD
      CPPSP=CPPS*CPD
      CPPPP=CPPP*CPD
      OQOX=S3*CDX
      OQOY=S3*CDY
      OQOZ=S3*CDZ
      OQXX=OQXO*OQOX+H*A34I
      OQYY=OQYO*OQOY+H*A34I
      OQZZ=OQZO*OQOZ+H*A34I
      OQXY=OQXO*OQOY
      OQXZ=OQXO*OQOZ
      OQYX=OQXY
      OQYZ=OQYO*OQOZ
      OQZX=OQXZ
      OQZY=OQYZ
      OQXOX=OQXO+OQOX
      OQYOY=OQYO+OQOY
      OQZOZ=OQZO+OQOZ
   52 CONTINUE
C
C        BEGIN THE FIRST PASS THROUGH THE INTEGRAL CALCULATIONS
C
      FLAG=.FALSE.
 3001 CONTINUE
C
C     INTEGRAL EVALUATION SECTION
C
C        (SS,SS) SECTION
C
      GOOOO=FQ0
      V0000=GOOOO
      VE00=V0000*EOOOO
      IF(JTYPE.LT.2) GO TO 4000
C
C        (PS,SS) SECTION
C
      QFQ1=-QA2*FQ1
      GXOOO=PQX*QFQ1
      V1000=OPXO*GOOOO+GXOOO
      GYOOO=PQY*QFQ1
      V2000=OPYO*GOOOO+GYOOO
      GZOOO=PQZ*QFQ1
      V3000=OPZO*GOOOO+GZOOO
      CPSSS=CPSS*CSD
      VE00=VE00+(V1000*E(65)+V2000*E(129)+V3000*E(193))*CPSSS
      TEMP=V0000*CPSSS
      VE11=TEMP*E( 65)
      VE21=TEMP*E(129)
      VE31=TEMP*E(193)
      IF(JTYPE-3) 4000,4003,4004
C
C        (PS,PS) + (SS,PS) SECTION
C
 4004 CONTINUE
      QFQ1=QA1*FQ1
      GOOXO=PQX*QFQ1
      V0010=OQXO*GOOOO+GOOXO
      GOOYO=PQY*QFQ1
      V0020=OQYO*GOOOO+GOOYO
      GOOZO=PQZ*QFQ1
      V0030=OQZO*GOOOO+GOOZO
      HFQ1=H*FQ1*A1234I
      QFQ2=-QA*FQ2*A1234I
      GXOXO=PQXX*QFQ2+HFQ1
      V1010=OPXO*V0010+OQXO*GXOOO+GXOXO
      GYOYO=PQYY*QFQ2+HFQ1
      V2020=OPYO*V0020+OQYO*GYOOO+GYOYO
      GZOZO=PQZZ*QFQ2+HFQ1
      V3030=OPZO*V0030+OQZO*GZOOO+GZOZO
      GXOYO=PQXY*QFQ2
      V1020=OPXO*V0020+OQYO*GXOOO+GXOYO
      V2010=OPYO*V0010+OQXO*GYOOO+GXOYO
      GXOZO=PQXZ*QFQ2
      V1030=OPXO*V0030+OQZO*GXOOO+GXOZO
      V3010=OPZO*V0010+OQXO*GZOOO+GXOZO
      GYOZO=PQYZ*QFQ2
      V2030=OPYO*V0030+OQZO*GYOOO+GYOZO
      V3020=OPZO*V0020+OQYO*GZOOO+GYOZO
      VE00=VE00+(V0010*E(5)+V0020*E(9)+V0030*E(13))*CSSPS
      TEMP=V0000*CSSPS
      VE13=TEMP*E(  5)
      VE23=TEMP*E(  9)
      VE33=TEMP*E( 13)
      VE00=VE00+(V1010*E( 69)+V1020*E(73)+V1030*E( 77)
     *         +V2010*E(133)+V2020*E(137)+V2030*E(141)
     *         +V3010*E(197)+V3020*E(201)+V3030*E(205))*CPSPS
      VE11=VE11+(V0010*E( 69)+V0020*E( 73)+V0030*E( 77))*CPSPS
      VE13=VE13+(V1000*E( 69)+V2000*E(133)+V3000*E(197))*CPSPS
      VE21=VE21+(V0010*E(133)+V0020*E(137)+V0030*E(141))*CPSPS
      VE23=VE23+(V1000*E( 73)+V2000*E(137)+V3000*E(201))*CPSPS
      VE31=VE31+(V0010*E(197)+V0020*E(201)+V0030*E(205))*CPSPS
      VE33=VE33+(V1000*E( 77)+V2000*E(141)+V3000*E(205))*CPSPS
      IF(JTYPE.EQ.4) GO TO 4000
C
C        (PP,SS) + (SP,SS) SECTION
C
 4003 CONTINUE
      V0100=OPOX*GOOOO+GXOOO
      V0200=OPOY*GOOOO+GYOOO
      V0300=OPOZ*GOOOO+GZOOO
      CSPSS=CSPS*CSD
      VE00=VE00+(V0100*E( 17)+V0200*E( 33)+V0300*E( 49))*CSPSS
      TEMP=V0000*CSPSS
      VE12=TEMP*E( 17)
      VE22=TEMP*E( 33)
      VE32=TEMP*E( 49)
      HFQ1=-H*QA2*FQ1*A12I
      QFQ2=QA2*QA2*FQ2
      GXXOO=PQXX*QFQ2+HFQ1
      V1100=OPXX*GOOOO+OPXOX*GXOOO+GXXOO
      GYYOO=PQYY*QFQ2+HFQ1
      V2200=OPYY*GOOOO+OPYOY*GYOOO+GYYOO
      GZZOO=PQZZ*QFQ2+HFQ1
      V3300=OPZZ*GOOOO+OPZOZ*GZOOO+GZZOO
      GXYOO=PQXY*QFQ2
      V1200=OPXO*V0200+OPOY*GXOOO+GXYOO
      V2100=OPYO*V0100+OPOX*GYOOO+GXYOO
      GXZOO=PQXZ*QFQ2
      V1300=OPXO*V0300+OPOZ*GXOOO+GXZOO
      V3100=OPZO*V0100+OPOX*GZOOO+GXZOO
      GYZOO=PQYZ*QFQ2
      V2300=OPYO*V0300+OPOZ*GYOOO+GYZOO
      V3200=OPZO*V0200+OPOY*GZOOO+GYZOO
      CPPSS=CPPS*CSD
      VE00=VE00+(V1100*E( 81)+V1200*E( 97)+V1300*E(113)
     *         + V2100*E(145)+V2200*E(161)+V2300*E(177)
     *         + V3100*E(209)+V3200*E(225)+V3300*E(241))*CPPSS
      VE11=VE11+(V0100*E( 81)+V0200*E( 97)+V0300*E(113))*CPPSS
      VE21=VE21+(V0100*E(145)+V0200*E(161)+V0300*E(177))*CPPSS
      VE31=VE31+(V0100*E(209)+V0200*E(225)+V0300*E(241))*CPPSS
      VE12=VE12+(V1000*E( 81)+V2000*E(145)+V3000*E(209))*CPPSS
      VE22=VE22+(V1000*E( 97)+V2000*E(161)+V3000*E(225))*CPPSS
      VE32=VE32+(V1000*E(113)+V2000*E(177)+V3000*E(241))*CPPSS
      IF(JTYPE.EQ.3) GO TO 4000
C
C        (PP,PS) + (SP,PS) SECTION
C
      V0110=OQXO*V0100+OPOX*GOOXO+GXOXO
      V0120=OQYO*V0100+OPOX*GOOYO+GXOYO
      V0130=OQZO*V0100+OPOX*GOOZO+GXOZO
      V0210=OQXO*V0200+OPOY*GOOXO+GXOYO
      V0220=OQYO*V0200+OPOY*GOOYO+GYOYO
      V0230=OQZO*V0200+OPOY*GOOZO+GYOZO
      V0310=OQXO*V0300+OPOZ*GOOXO+GXOZO
      V0320=OQYO*V0300+OPOZ*GOOYO+GYOZO
      V0330=OQZO*V0300+OPOZ*GOOZO+GZOZO
      CSPPS=CSPP*CSD
      VE00=VE00+(V0110*E( 21)+V0120*E( 25)+V0130*E( 29)
     *         + V0210*E( 37)+V0220*E( 41)+V0230*E( 45)
     *         + V0310*E( 53)+V0320*E( 57)+V0330*E( 61))*CSPPS
      VE12=VE12+(V0010*E( 21)+V0020*E( 25)+V0030*E( 29))*CSPPS
      VE22=VE22+(V0010*E( 37)+V0020*E( 41)+V0030*E( 45))*CSPPS
      VE32=VE32+(V0010*E( 53)+V0020*E( 57)+V0030*E( 61))*CSPPS
      VE13=VE13+(V0100*E( 21)+V0200*E( 37)+V0300*E( 53))*CSPPS
      VE23=VE23+(V0100*E( 25)+V0200*E( 41)+V0300*E( 57))*CSPPS
      VE33=VE33+(V0100*E( 29)+V0200*E( 45)+V0300*E( 61))*CSPPS
      QFQ3=QA1*QA2*QA2*FQ3
      HFQ2=-H*QA2*FQ2*A1234I
      TFQ2=THREE*HFQ2
      GXYZO=PQXY*PQZ*QFQ3
      C1230=OPXY*GOOZO+OPXO*GYOZO+OPOY*GXOZO+GXYZO
      V1230=OQZO*V1200+C1230
      C1320=OPXZ*GOOYO+OPXO*GYOZO+OPOZ*GXOYO+GXYZO
      V1320=OQYO*V1300+C1320
      C2130=OPYX*GOOZO+OPYO*GXOZO+OPOX*GYOZO+GXYZO
      V2130=OQZO*V2100+C2130
      C2310=OPYZ*GOOXO+OPYO*GXOZO+OPOZ*GXOYO+GXYZO
      V2310=OQXO*V2300+C2310
      C3120=OPZX*GOOYO+OPZO*GXOYO+OPOX*GYOZO+GXYZO
      V3120=OQYO*V3100+C3120
      C3210=OPZY*GOOXO+OPZO*GXOYO+OPOY*GXOZO+GXYZO
      V3210=OQXO*V3200+C3210
      TEMP=PQXX*QFQ3
      GXXXO=PQX*(TEMP+TFQ2)
      C1110=OPXX*GOOXO+OPXOX*GXOXO+GXXXO
      V1110=OQXO*V1100+C1110
      GXXYO=PQY*(TEMP+HFQ2)
      C1120=OPXX*GOOYO+OPXOX*GXOYO+GXXYO
      V1120=OQYO*V1100+C1120
      C1210=OPXY*GOOXO+OPXO*GXOYO+OPOY*GXOXO+GXXYO
      V1210=OQXO*V1200+C1210
      C2110=OPYX*GOOXO+OPYO*GXOXO+OPOX*GXOYO+GXXYO
      V2110=OQXO*V2100+C2110
      GXXZO=PQZ*(TEMP+HFQ2)
      C1130=OPXX*GOOZO+OPXOX*GXOZO+GXXZO
      V1130=OQZO*V1100+C1130
      C1310=OPXZ*GOOXO+OPXO*GXOZO+OPOZ*GXOXO+GXXZO
      V1310=OQXO*V1300+C1310
      C3110=OPZX*GOOXO+OPZO*GXOXO+OPOX*GXOZO+GXXZO
      V3110=OQXO*V3100+C3110
      TEMP=PQYY*QFQ3
      GYYYO=PQY*(TEMP+TFQ2)
      C2220=OPYY*GOOYO+OPYOY*GYOYO+GYYYO
      V2220=OQYO*V2200+C2220
      GYYXO=PQX*(TEMP+HFQ2)
      C2210=OPYY*GOOXO+OPYOY*GXOYO+GYYXO
      V2210=OQXO*V2200+C2210
      C2120=OPYX*GOOYO+OPYO*GXOYO+OPOX*GYOYO+GYYXO
      V2120=OQYO*V2100+C2120
      C1220=OPXY*GOOYO+OPXO*GYOYO+OPOY*GXOYO+GYYXO
      V1220=OQYO*V1200+C1220
      GYYZO=PQZ*(TEMP+HFQ2)
      C2230=OPYY*GOOZO+OPYOY*GYOZO+GYYZO
      V2230=OQZO*V2200+C2230
      C2320=OPYZ*GOOYO+OPYO*GYOZO+OPOZ*GYOYO+GYYZO
      V2320=OQYO*V2300+C2320
      C3220=OPZY*GOOYO+OPZO*GYOYO+OPOY*GYOZO+GYYZO
      V3220=OQYO*V3200+C3220
      TEMP=PQZZ*QFQ3
      GZZZO=PQZ*(TEMP+TFQ2)
      C3330=OPZZ*GOOZO+OPZOZ*GZOZO+GZZZO
      V3330=OQZO*V3300+C3330
      GZZXO=PQX*(TEMP+HFQ2)
      C3310=OPZZ*GOOXO+OPZOZ*GXOZO+GZZXO
      V3310=OQXO*V3300+C3310
      C3130=OPZX*GOOZO+OPZO*GXOZO+OPOX*GZOZO+GZZXO
      V3130=OQZO*V3100+C3130
      C1330=OPXZ*GOOZO+OPXO*GZOZO+OPOZ*GXOZO+GZZXO
      V1330=OQZO*V1300+C1330
      GZZYO=PQY*(TEMP+HFQ2)
      C3320=OPZZ*GOOYO+OPZOZ*GYOZO+GZZYO
      V3320=OQYO*V3300+C3320
      C3230=OPZY*GOOZO+OPZO*GYOZO+OPOY*GZOZO+GZZYO
      V3230=OQZO*V3200+C3230
      C2330=OPYZ*GOOZO+OPYO*GZOZO+OPOZ*GYOZO+GZZYO
      V2330=OQZO*V2300+C2330
      CPPPS=CPPP*CSD
      VE00=VE00+(V1110*E( 85)+V1120*E( 89)+V1130*E( 93)
     *          +V1210*E(101)+V1220*E(105)+V1230*E(109)
     *          +V1310*E(117)+V1320*E(121)+V1330*E(125)
     *          +V2110*E(149)+V2120*E(153)+V2130*E(157)
     *          +V2210*E(165)+V2220*E(169)+V2230*E(173)
     *          +V2310*E(181)+V2320*E(185)+V2330*E(189)
     *          +V3110*E(213)+V3120*E(217)+V3130*E(221)
     *          +V3210*E(229)+V3220*E(233)+V3230*E(237)
     *          +V3310*E(245)+V3320*E(249)+V3330*E(253))*CPPPS
      VE11=VE11+(V0110*E( 85)+V0120*E( 89)+V0130*E( 93)
     *         + V0210*E(101)+V0220*E(105)+V0230*E(109)
     *         + V0310*E(117)+V0320*E(121)+V0330*E(125))*CPPPS
      VE12=VE12+(V1010*E( 85)+V1020*E( 89)+V1030*E( 93)
     *         + V2010*E(149)+V2020*E(153)+V2030*E(157)
     *         + V3010*E(213)+V3020*E(217)+V3030*E(221))*CPPPS
      VE13=VE13+(V1100*E( 85)+V1200*E(101)+V1300*E(117)
     *         + V2100*E(149)+V2200*E(165)+V2300*E(181)
     *         + V3100*E(213)+V3200*E(229)+V3300*E(245))*CPPPS
      VE21=VE21+(V0110*E(149)+V0120*E(153)+V0130*E(157)
     *         + V0210*E(165)+V0220*E(169)+V0230*E(173)
     *         + V0310*E(181)+V0320*E(185)+V0330*E(189))*CPPPS
      VE22=VE22+(V1010*E(101)+V1020*E(105)+V1030*E(109)
     *         + V2010*E(165)+V2020*E(169)+V2030*E(173)
     *         + V3010*E(229)+V3020*E(233)+V3030*E(237))*CPPPS
      VE23=VE23+(V1100*E( 89)+V1200*E(105)+V1300*E(121)
     *         + V2100*E(153)+V2200*E(169)+V2300*E(185)
     *         + V3100*E(217)+V3200*E(233)+V3300*E(249))*CPPPS
      VE31=VE31+(V0110*E(213)+V0120*E(217)+V0130*E(221)
     *         + V0210*E(229)+V0220*E(233)+V0230*E(237)
     *         + V0310*E(245)+V0320*E(249)+V0330*E(253))*CPPPS
      VE32=VE32+(V1010*E(117)+V1020*E(121)+V1030*E(125)
     *         + V2010*E(181)+V2020*E(185)+V2030*E(189)
     *         + V3010*E(245)+V3020*E(249)+V3030*E(253))*CPPPS
      VE33=VE33+(V1100*E( 93)+V1200*E(109)+V1300*E(125)
     *         + V2100*E(157)+V2200*E(173)+V2300*E(189)
     *         + V3100*E(221)+V3200*E(237)+V3300*E(253))*CPPPS
      IF(JTYPE.NE.6) GO TO 4000
C
C        (PP,PP), (PP,SP), (PS,PP), (SP,PP), (SS,PP), (SP,SP), (PS,SP),
C        AND (SS,SP) SECTION
C
      CALL FPPPP
 4000 CONTINUE
      IF(FLAG) GO TO 3002
C
C        END OF THE FIRST PASS THROUGH THE INTEGRAL SECTION
C
      FLAG=.TRUE.
      FQ0=FQ1
      FQ1=FQ2
      FQ2=FQ3
      FQ3=FQ4
      FQ4=FQ5
      VE00S=VE00S+VE00
      VE11S=VE11S+VE11
      VE21S=VE21S+VE21
      VE31S=VE31S+VE31
      VE12S=VE12S+VE12
      VE22S=VE22S+VE22
      VE32S=VE32S+VE32
      CDVE00=S34*(VE00+VE00)
      DX2X=-VE13*S4+VE14*S3-CDX*CDVE00
      DX2Y=-VE23*S4+VE24*S3-CDY*CDVE00
      DX2Z=-VE33*S4+VE34*S3-CDZ*CDVE00
C
C        BRANCH TO SECOND PASS
C
      GO TO 3001
C
C        END OF THE SECOND PASS THROUGH THE INTEGRAL SECTION
C
 3002 CONTINUE
      QVE00=QA*(VE00+VE00)
      DVEX=-(VE11+VE12)*QA2+(VE13+VE14)*QA1-PQX*QVE00
      DVEY=-(VE21+VE22)*QA2+(VE23+VE24)*QA1-PQY*QVE00
      DVEZ=-(VE31+VE32)*QA2+(VE33+VE34)*QA1-PQZ*QVE00
      DVEXS=DVEXS+DVEX
      DVEYS=DVEYS+DVEY
      DVEZS=DVEZS+DVEZ
      GO TO 5000
C
C        SPECIAL (SS,SS) SECTION
C
 3003 CONTINUE
      VE00=FQ0*EOOOO
      VE00S=VE00S+VE00
      CDVE00=S34*(VE00+VE00)
      DX2X=-CDX*CDVE00
      DX2Y=-CDY*CDVE00
      DX2Z=-CDZ*CDVE00
      QVE00=QA*FQ1*(EOOOO+EOOOO)
      DVEX=-PQX*QVE00
      DVEY=-PQY*QVE00
      DVEZ=-PQZ*QVE00
      DVEXS=DVEXS+DVEX
      DVEYS=DVEYS+DVEY
      DVEZS=DVEZS+DVEZ
      GO TO 5000
C
C        SPECIAL (PS,SS) SECTION
C
 3004 CONTINUE
      CPSSS=CPSS*CSD
      EXOOO =E( 65)*CPSSS
      EYOOO =E(129)*CPSSS
      EZOOO =E(193)*CPSSS
      TEMP1=OPXO*EXOOO +OPYO*EYOOO +OPZO*EZOOO
      TEMP2=-(PQX *EXOOO +PQY *EYOOO +PQZ *EZOOO )*QA2
      VE00=FQ0*(EOOOO+TEMP1)+FQ1*TEMP2
      VE00S=VE00S+VE00
      VE11S=VE11S+FQ0*EXOOO
      VE21S=VE21S+FQ0*EYOOO
      VE31S=VE31S+FQ0*EZOOO
      CDVE00=S34*(VE00+VE00)
      DX2X=-CDX*CDVE00
      DX2Y=-CDY*CDVE00
      DX2Z=-CDZ*CDVE00
      VE00=FQ1*(EOOOO+TEMP1)+FQ2*TEMP2
      QVE00=QA*(VE00+VE00)
      TEMP=-QA2*FQ1
      DVEX=-PQX*QVE00+TEMP*EXOOO
      DVEY=-PQY*QVE00+TEMP*EYOOO
      DVEZ=-PQZ*QVE00+TEMP*EZOOO
      DVEXS=DVEXS+DVEX
      DVEYS=DVEYS+DVEY
      DVEZS=DVEZS+DVEZ
 5000 CONTINUE
C
C        SUMMATION OF CONTRIBUTIONS FROM THE UNCONTRACTED GAUSSIANS
C
      FXK=FXK+DX2X-DVEX*S3
      FYK=FYK+DX2Y-DVEY*S3
      FZK=FZK+DX2Z-DVEZ*S3
 2004 CONTINUE
 2003 CONTINUE
      VEE=VEE+VE00S
      ABVE00=S12*(VE00S+VE00S)
      DX1X=-VE11S*S2+VE12S*S1-ABX*ABVE00
      DX1Y=-VE21S*S2+VE22S*S1-ABY*ABVE00
      DX1Z=-VE31S*S2+VE32S*S1-ABZ*ABVE00
      FXI=FXI+DX1X+DVEXS*S1
      FYI=FYI+DX1Y+DVEYS*S1
      FZI=FZI+DX1Z+DVEZS*S1
      FXJ=FXJ-DX1X+DVEXS*S2
      FYJ=FYJ-DX1Y+DVEYS*S2
      FZJ=FZJ-DX1Z+DVEZS*S2
 2002 CONTINUE
 2001 CONTINUE
C
C        SUMMATION OF THE CONTRIBUTIONS FROM THE SHELLS
C
      FXL=-(FXI+FXJ+FXK)
      FYL=-(FYI+FYJ+FYK)
      FZL=-(FZI+FZJ+FZK)
      FXYZ(IATX)=FXYZ(IATX)+FXI
      FXYZ(JATX)=FXYZ(JATX)+FXJ
      FXYZ(KATX)=FXYZ(KATX)+FXK
      FXYZ(LATX)=FXYZ(LATX)+FXL
      FXYZ(IATY)=FXYZ(IATY)+FYI
      FXYZ(JATY)=FXYZ(JATY)+FYJ
      FXYZ(KATY)=FXYZ(KATY)+FYK
      FXYZ(LATY)=FXYZ(LATY)+FYL
      FXYZ(IATZ)=FXYZ(IATZ)+FZI
      FXYZ(JATZ)=FXYZ(JATZ)+FZJ
      FXYZ(KATZ)=FXYZ(KATZ)+FZK
      FXYZ(LATZ)=FXYZ(LATZ)+FZL
      IF(IDUMP.GE.2)
     *WRITE(IOUT,2005) ISHELL,JSHELL,KSHELL,LSHELL,FXI,FXJ,FXK,FXL,
     * FYI,FYJ,FYK,FYL,FZI,FZJ,FZK,FZL
      IF(.NOT.USESYM) GOTO 1004
C
C     SUMMATION OF CONTRIBUTIONS FROM SYMMETRICALLY EQUIVALENT SHELLS.
C
      DO 300 KOP=2,NSYMOP
      JOP = ISYMOP(KOP)
      IF(JOP .EQ. 0) GOTO 300
      JOPIND=(JOP-1)*NATOMS
      IAT1 = NEQATM(IAT+JOPIND)
      JAT1 = NEQATM(JAT+JOPIND)
      KAT1 = NEQATM(KAT+JOPIND)
      LAT1 = NEQATM(LAT+JOPIND)
      IATX = 3*IAT1 - 2
      JATX = 3*JAT1 - 2
      KATX = 3*KAT1 - 2
      LATX = 3*LAT1 - 2
      IATY = IATX + 1
      JATY = JATX + 1
      KATY = KATX + 1
      LATY = LATX + 1
      IATZ = IATY + 1
      JATZ = JATY + 1
      KATZ = KATY + 1
      LATZ = LATY + 1
      IXTR = JTRANS(1,JOP)
      IYTR = JTRANS(2,JOP)
      IZTR = JTRANS(3,JOP)
      FXYZ(IATX) = FXYZ(IATX) + FXI*IXTR
      FXYZ(JATX) = FXYZ(JATX) + FXJ*IXTR
      FXYZ(KATX) = FXYZ(KATX) + FXK*IXTR
      FXYZ(LATX) = FXYZ(LATX) + FXL*IXTR
      FXYZ(IATY) = FXYZ(IATY) + FYI*IYTR
      FXYZ(JATY) = FXYZ(JATY) + FYJ*IYTR
      FXYZ(KATY) = FXYZ(KATY) + FYK*IYTR
      FXYZ(LATY) = FXYZ(LATY) + FYL*IYTR
      FXYZ(IATZ) = FXYZ(IATZ) + FZI*IZTR
      FXYZ(JATZ) = FXYZ(JATZ) + FZJ*IZTR
      FXYZ(KATZ) = FXYZ(KATZ) + FZK*IZTR
      FXYZ(LATZ) = FXYZ(LATZ) + FZL*IZTR
      IF(IDUMP.GE.2)WRITE(IOUT,2006) JOP,IXTR,IYTR,IZTR
  300 CONTINUE
 1004 CONTINUE
 1003 CONTINUE
 1002 CONTINUE
 1001 CONTINUE
C***
      DO 9987 I=1,NATOMS
 9987 WRITE(IOUT,9986) (FXYZ(3*I-3+J),J=1,3)
 9986 FORMAT(2X,3F18.9)
C***      IF(IDUMP.GT.0) CALL DMPFRC(IOUT,6HTWLDRV,6,NATOMS,F1XYZ)
      IF(IFMT.NE.0) WRITE(IOUT,2007) IFMT
C***      CALL TRAKIO(6HTWLDRV,2)
      RETURN
      END