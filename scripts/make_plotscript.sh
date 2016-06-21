#!/bin/bash

PFILE=./plotdata/plot_script.gp
FILES=$(ls ./sim_results/)
BAK_DIR=$(ls ./plotdata/ | less | grep "bak")

i=0
for bd in $BAK_DIR
  do
  i=$(echo "$i+1" | bc)
done
mkdir ./plotdata/bak$i
mv ./plotdata/2D ./plotdata/bak$i/
mv ./plotdata/3D ./plotdata/bak$i/

mkdir ./plotdata/2D
mkdir ./plotdata/3D
mkdir ./plotdata/2D/plots
mkdir ./plotdata/3D/plots


echo "#Plot script" > $PFILE
echo "set view 60,15" >> $PFILE
echo "set term postscript eps enhanced 'Helvetica' 22" >> $PFILE
echo "set key rmargin center" >> $PFILE
echo "set style line 1 lw 3 pt 2" >> $PFILE # linewidth x3 and point type a cross
echo "set style line 2 lw 3 pt 5" >> $PFILE  # linewidth x3 and point type a filled square
echo "set style line 3 lw 3 pt 6" >> $PFILE  # linewidth x3 and point type an empty circle


for f in $FILES
  do
  echo "$f"
  
  type=$(echo "$f" | cut -d"." -f3)
  
  ns=$(tail -n 1 ./sim_results/$f | cut -d" " -f3)

  #for s in randInt fixInt
  #  do
           
  k=0
  case "$type" in

      delay )
      # Prepare 2D files
      for nw in 10 20 40
	do
	for nr in 10 20 40 80
	  do
	  fl=./plotdata/2D/RD.nr$nr.nw$nw.$f.2D
	  cat ./sim_results/$f | \
	      awk -v r=$nr -v w=$nw '{if ($4 == r && $5 == w) { print; } }' > $fl

	  fl=./plotdata/2D/WD.nr$nr.nw$nw.$f.2D
	  cat ./sim_results/$f | \
	      awk -v r=$nr -v w=$nw '{if ($4 == r && $5 == w) { print; } }' > $fl
	done #readers
      done #writers

      # Prepare 3D files
      for p in SIMPLE CWFR SSO SSO_APRX
	do
	for nw in 10 20 40 80
	  do
	  fl=./plotdata/3D/RD.$p.nw$nw.$f.3D
	  grep "$p " ./sim_results/$f | \
	      awk -v w=$nw '{ if ($5 == w) { i++; if ( i%4 == 0 ) { printf("\n"); print; i++;} else { print; } } }' > $fl
	done #writers
	for nr in 10 20 40 80
	  do
	  fl=./plotdata/3D/WD.$p.nr$nr.$f.3D
	  grep $p ./sim_results/$f | \
	      awk -v r=$nr '{ if ($4 == r) { i++; if ( i%4 == 0 ) { printf("\n"); print; i++;} else { print;  } } }' > $fl
	done #writers
      done #proto
      ;;

      rounds )
      # Read Latency vs #Readers  
      for nw in 10 20 40 80
	do
	rr=./plotdata/2D/RR.nw$nw.$f.2D
	cat ./sim_results/$f | \
	    awk -v w=$nw '{if ($5 == w) { print; } }' > $rr
	
	rl=./plotdata/2D/RL.nw$nw.$f.2D
	cat ./sim_results/$f | \
	    awk -v w=$nw '{if ($5 == w) { print; } }' > $rl
      done #writers

      # Write Latency vs #Writers  
      for nr in 10 20 40 80
	do
	wr=./plotdata/2D/WR.nr$nr.$f.2D
	cat ./sim_results/$f | \
	    awk -v r=$nr '{if ($4 == r) { print; } }' > $wr
	
	wl=./plotdata/2D/WL.nr$nr.$f.2D
	cat ./sim_results/$f | \
	    awk -v r=$nr '{if ($4 == r) { print; } }' > $wl
      done #writers

       # Read Latency vs #Servers  
      nf=$(echo $f | cut -d'.' -f5)
      t=$(echo $f | cut -d'.' -f6)
      for nr in 10 20 40 80
      do
	for nw in 10 20 40 80
	do
	  sr=./plotdata/2D/SR.nr$nr.nw$nw.$nf.$t.2D
	  cat ./sim_results/$f | \
	      awk -v r=$nr -v w=$nw '{if ($4 == r && $5 == w) { print; } }' >> $sr

	  sl=./plotdata/2D/SL.nr$nr.nw$nw.$nf.$t.2D
	  cat ./sim_results/$f | \
	      awk -v r=$nr -v w=$nw '{if ($4 == r && $5 == w) { print; } }' >> $sl
	done
      done
      ;;

      * )
      echo "Hello World!!"
      ;;
  esac

  FLIST=$(ls ./plotdata/2D/ | less | grep "$f")
  
  for fl in $FLIST
    do
    echo "$fl"
    
    type=$(echo $fl | cut -d"." -f1)

    #echo $type

    #Create 2D plots
    echo "set output \"2D/plots/$fl.eps\"" >> $PFILE
    
    # this case generates the appropriate labels and specifies the columns of the plot
    case "$type" in
	RD )
	#xlab=$(head -n 1 ./sim_results/$f| cut -d" " -f9)
	#ylab=$(head -n 1 ./sim_results/$f| cut -d" " -f15)
        #zlab=$(head -n 1 ./sim_results/$f| cut -d" " -f9)
	xlab=Delay
	ylab="Read Latency (seconds)"

	# Have Gnuplot determine ranges
	echo "set autoscale" >> $PFILE

	ti="Read Latency vs Msg Delay"
	x=9
	y=15
	;;

	WD )
	xlab=Delay
	ylab="Write Latency (seconds)"

	# Have Gnuplot determine ranges
	echo "set autoscale" >> $PFILE

	ti="Write Latency vs Msg Delay"
	x=9
	y=21
	;;

	RR )
	xlab=#Readers
	ylab=%2comm-reads

	# set the range of the y axis above 100%
	echo "set yrange [0:110]" >> $PFILE

	ti="% of Slow Reads vs # of Readers"
	x=4
	y=13
	;;

	RL )
	xlab=#Readers
	ylab="Read Latency (seconds)"

	# Have Gnuplot determine ranges
	echo "set autoscale" >> $PFILE

	ti="Read Latency vs # of Readers"
	x=4
	y=14
	;;

	WR )
	xlab=#Writers
	ylab=%2comm-writes

	# set the range of the y axis above 100%
	echo "set yrange [0:110]" >> $PFILE

	ti="% of Slow Writes vs # of Writers"
	x=5
	y=19
	;;

	WL )
	xlab=#Writers
	ylab="Write Latency (seconds)"

	# Have Gnuplot determine ranges
	echo "set autoscale" >> $PFILE

	ti="Write Latency vs # of Writers"
	x=5
	y=20
	;;
    esac

    #title=$(echo "$ti: $fl plot") 
    title=$(echo "$ti") 
    echo "set title \"$title\"" >> $PFILE
    echo "set xlabel \"$xlab\"" >> $PFILE 
    echo "set ylabel \"$ylab\"" >> $PFILE 
    echo "plot \"< grep SIMPLE 2D/$fl\" using $x:$y title \"MW-ABD\" with linespoints ls 3, \\" >> $PFILE
    echo "\"< grep CWFR 2D/$fl\" using $x:$y  title \"CWFR\" with linespoints ls 2,\\" >> $PFILE
    #echo "\"< grep 'SSO ' 2D/$fl\" using $x:$y  title \"SFW\" with linespoints, \\" >> $PFILE 
    echo "\"< grep 'SSO_APRX' 2D/$fl\"  using $x:$y title \"APRX-SFW\" with linespoints ls 1" >> $PFILE
    echo "" >> $PFILE 
  done
    
  FLIST=$(ls ./plotdata/3D/ | less | grep "$f")
  
  for fl in $FLIST
    do
    
    echo "$fl"
       
    type=$(echo $fl | cut -d"." -f1)

    #echo $type

    #Create 3D plots
    #echo "set term postscript eps enhanced" >> $PFILE
    echo "set output \"3D/plots/$fl.eps\"" >> $PFILE

    
    # this case generates the appropriate labels and specifies the columns of the plot
    case "$type" in
	RD )
	#xlab=$(head -n 1 ./sim_results/$f| cut -d" " -f9)
	#ylab=$(head -n 1 ./sim_results/$f| cut -d" " -f15)
        #zlab=$(head -n 1 ./sim_results/$f| cut -d" " -f9)
	xlab=Delay
	ylab=#Readers
	zlab="Read Latency (seconds)"

	# Have Gnuplot determine ranges
	echo "set autoscale" >> $PFILE
	
	ti="#Readers vs Read Latency vs Msg Delay"
	x=9
	y=4
	z=15
	;;

	WD )
	xlab=Delay
	ylab=#Writers
	zlab="Write Latency (seconds)"

	# Have Gnuplot determine ranges
	echo "set autoscale" >> $PFILE
	
	ti="#Writers vs Read Latency vs Msg Delay"
	x=9
	y=5
	z=21
	;;

	RR )
	xlab=#Readers
	ylab=%2comm-reads

	# set the range of the y axis above 100%
	echo "set yrange [0:110]" >> $PFILE

	ti="% of Slow Reads vs # of Readers"
	x=4
	y=13
	;;

	RL )
	xlab=#Readers
	ylab="Read Latency (seconds)"
	ti="Read Latency vs # of Readers"
	x=4
	y=14
	;;

	WR )
	xlab=#Writers
	ylab=%2comm-writes

	# set the range of the y axis above 100%
	echo "set yrange [0:110]" >> $PFILE

	ti="% of Slow Writes vs # of Writers"
	x=5
	y=19
	;;

	WL )
	xlab=#Writers
	ylab="Write Latency (seconds)"
	ti="Write Latency vs # of Writers"
	x=5
	y=20
	;;

    esac

    #title=$(echo "$ti: $fl plot") 
    title=$(echo "$ti") 
    echo "set title \"$title\"" >> $PFILE
    echo "set xlabel \"$xlab\"" >> $PFILE
    echo "set ylabel \"$ylab\"" >> $PFILE
    echo "set zlabel \"$zlab\"" >> $PFILE
    echo "splot \"3D/$fl\" using $x:$y:$z with lines" >> $PFILE
    echo "" >> $PFILE
  done
  #done
done

# Create the plots with regard to the # of servers
  FLIST=$(ls ./plotdata/2D/ | less | grep 'nr[0-9]*.nw[0-9]*')
  
  for fl in $FLIST
    do
    echo "$fl"
    
    type=$(echo $fl | cut -d"." -f1)

    #echo $type

    for role in R W 
      do
    #Create 2D plots
      #echo "set term postscript eps enhanced" >> $PFILE
      echo "set output \"2D/plots/$fl.$role.eps\"" >> $PFILE
      
    # this case generates the appropriate labels and specifies the columns of the plot
      case "$type" in
	  SR )
	  xlab=#Quorums
	  x=6
	  
	  case "$role" in
	      R )
	  #if [ "$role"="R" ]; then
	      ylab=%2comm-reads

	      #set the range of the y axis above 100%
	      echo "set yrange [0:110]" >> $PFILE
	      
	      ti="% of Slow Reads vs # of Servers"
	      y=13
	      ;;
	      
	      W )
	      ylab=%2comm-writes

	      # set the range of the y axis above 100%
	      echo "set yrange [0:110]" >> $PFILE
	      
	      ti="% of Slow Writes vs # of Servers"
	      y=19
	      ;;
	  esac
	  ;;
	  
	  SL )
	  xlab=#Quorums
	  x=6
	  case "$role" in
	      R )
	  #if [ "$role"="R" ]; then
	      ylab="Read Latency (seconds)"

	      #Have Gnuplot determine ranges
	      echo "set autoscale" >> $PFILE

	      ti="Read Latency vs # of Servers"
	      y=14
	      ;;
	  #else
	      W )
	      ylab="Write Latency (seconds)"

	      #Have Gnuplot determine ranges
	      echo "set autoscale" >> $PFILE

	      ti="Write Latency vs # of Servers"
	      y=20
	      ;;
	  #fi
	  esac
	  ;;

      esac
      
      #title=$(echo "$ti: $fl plot") 
      title=$(echo "$ti") 
      echo "set title \"$title\"" >> $PFILE
      echo "set xlabel \"$xlab\"" >> $PFILE 
      echo "set ylabel \"$ylab\"" >> $PFILE 
      echo "plot \"< grep SIMPLE 2D/$fl\" using $x:$y title \"MW-ABD\" with linespoints ls 3, \\" >> $PFILE
      echo "\"< grep CWFR 2D/$fl\" using $x:$y  title \"CWFR\" with linespoints ls 2,\\" >> $PFILE
      #echo "\"< grep 'SSO ' 2D/$fl\" using $x:$y  title \"SFW\" with linespoints, \\" >> $PFILE 
      echo "\"< grep 'SSO_APRX' 2D/$fl\"  using $x:$y title \"APRX-SFW\" with linespoints ls 1" >> $PFILE
      echo "" >> $PFILE 
    done #role
  done