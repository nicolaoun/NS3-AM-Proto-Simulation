#!/bin/bash

NS=ns
nbTests=5
ops=25
delay=0.3
seed=0
q_file=quorum.maj25.f1.dat
numservers=25
fail=1


runtest(){

# copy the appropriate quorum file
    cp $q_file quorum_sys.dat

    
#do rand tests:
    
    tot1comm=0
    tot2comm=0
    tottime=0
    avetime=0
    seed=0

    wtot1comm=0
    wtot2comm=0
    wtottime=0
    wavetime=0

    #./quorum_gen -c $numservers $seed> $q_file

    q=$(wc -l quorum_sys.dat | cut -d" " -f1)
    #q=$(echo "$q-1" | bc)

    if [ $proto = 10 ]; then
	protostr=SIMPLE
    fi

    if [ $proto = 11 ]; then
	protostr=CWFR
    fi

    if [ $proto = 12 ]; then
	protostr=SSO
    fi

    if [ $proto = 13 ]; then
	protostr=SSO_APRX
    fi
    
    for ((i=1; i<=nbTests ; i++))
      do
      seed=$(echo "$seed+19" | bc)
      
      file=$OUTDIR/output-$runfile-ns$numservers-nr$numreaders-nw$numwriters-ri$ri-wi$wi-$protostr
      echo "Q=$q  ns=$numservers  nr=$numreaders file=$file wInt=$wi rInt=$ri run=$runfile data=$commdata seed=$seed operation=$ops delay=$delay"
      
      $NS $runfile.tcl $numservers $numreaders $numwriters $q $ri $wi $delay $ops $seed $proto > $file
      
      for((j=0; j<numwriters; j++))
	do
	#echo "j=$j file=$file" 
	wonecomm=$(grep "WRITER: $j " $file | cut -d" " -f6 | cut -d"," -f1)
	wtot1comm=$(echo "$wonecomm + $wtot1comm" | bc)
	wtwocomm=$(grep "WRITER: $j " $file | cut -d" " -f8 | cut -d"," -f1)
	wtot2comm=$(echo "$wtwocomm + $wtot2comm" | bc)
	wtime=$(grep "WRITER: $j " $file | cut -d" " -f10 | cut -d")" -f1)
	wtottime=$(echo "$wtime + $wtottime" | bc)
      done #j
      wavetime=$(echo "scale=4; $wavetime + ($wtottime / $numwriters)" | bc -l)
      echo "Wtot1comm= $wtot1comm, Wtot2comm = $wtot2comm Wavetime= $wavetime" >> $runout
      echo "Wtot1comm= $wtot1comm, Wtot2comm = $wtot2comm Wavetime= $wavetime"
      
      for((j=numwriters; j<numreaders+numwriters; j++))
	do
	#echo "j=$j file=$file" 
	onecomm=$(grep "READER: $j " $file | cut -d" " -f6 | cut -d"," -f1)
	tot1comm=$(echo "$onecomm + $tot1comm" | bc)
	twocomm=$(grep "READER: $j " $file | cut -d" " -f8 | cut -d"," -f1)
	tot2comm=$(echo "$twocomm + $tot2comm" | bc)
	time=$(grep "READER: $j " $file | cut -d" " -f10 | cut -d")" -f1)
	tottime=$(echo "$time + $tottime" | bc)
      done #j
      avetime=$(echo "scale=4; $avetime + ($tottime / $numreaders)" | bc -l)
      echo "Rtot1comm= $tot1comm, Rtot2comm = $tot2comm Ravetime= $avetime"
      echo "Rtot1comm= $tot1comm, Rtot2comm = $tot2comm Ravetime= $avetime" >> $runout
    done #nbTests
   
    # Compute data for Writers
    Wave2comm=$(echo "scale=4; $wtot2comm/$nbTests" | bc -l)
    Wratio=$(echo "scale=4; $wtot2comm/($wtot2comm+$wtot1comm)" | bc -l)
    Wpercentage=$(echo "scale=2; (100*$wtot2comm)/($wtot2comm+$wtot1comm)" | bc -l)
    Wtotavetime=$(echo "scale=4; $wavetime/$nbTests" | bc -l)
    echo "WAverage = $Wave2comm, WRatio=$Wratio, WAverageTime=$Wtotavetime"    
    echo "WAverage = $Wave2comm, WRatio=$Wratio, WAverageTime=$Wtotavetime" >> $runout   

    #Compute data for Readers
    Rave2comm=$(echo "scale=4; $tot2comm/$nbTests" | bc -l)
    Rratio=$(echo "scale=4; $tot2comm/($tot2comm+$tot1comm)" | bc -l)
    Rpercentage=$(echo "scale=2; (100*$tot2comm)/($tot2comm+$tot1comm)" | bc -l)
    Rtotavetime=$(echo "scale=4; $avetime/$nbTests" | bc -l)
    echo "RAverage = $Rave2comm, RRatio=$Rratio, RAverageTime=$Rtotavetime"
    echo "RAverage = $Rave2comm, RRatio=$Rratio, RAverageTime=$Rtotavetime" >> $runout

    echo "$protostr $runfile $numservers $numreaders $numwriters $q $ri $wi $tot1comm $tot2comm $Rave2comm $Rratio $Rpercentage $Rtotavetime $wtot1comm $wtot2comm $Wave2comm $Wratio $Wpercentage $Wtotavetime" >> $commdata

}


wi=4
#ri=4
numwriters=80
numreaders=10
runfile=randInt
r=23
for fail in 1 
  do
  for numservers in 10 15 20 25
    do
    commdata=all.PROTO.rounds.maj$numservers.f$fail.ri$r.data
    runout=run.out.maj$numservers.f$fail.ri$r.data
    
    mkdir outdata/out.maj$numservers.f$fail
    
    OUTDIR=outdata/out.maj$numservers.f$fail
    
    echo "Protocol Version #Servers #Readers #Writers #Q RInt WInt RTot1Comm RTot2Comm RAve2comm RRatio RPercentage RAveTime WTot1Comm WTot2Comm WAve2comm WRatio WPercentage WAveTime" > $commdata

    for proto in 10 11 13
      do
#      for ((r=23; r<=63; r=r+20))
#	do
	ri=$(echo "scale=1; $r/10.0" | bc)
	for ((numwriters=10; numwriters<=80; numwriters=numwriters*2))
	  do
	  for ((numreaders=10; numreaders<=80; numreaders=numreaders*2)) 
	    do
#	    runfile=$f
	    q_file=quorum.maj$numservers.f$fail.dat
	    runtest
	  done #numreaders
	done #numwriters
#      done #ri
    done #protocol
  done #numservers
done #file

exit 0
