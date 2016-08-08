#Variables
set opt(ns) 20;  #number of servers
set opt(nr) 80;  #number of readers
set opt(nt) 4;  #maximum number of faulty servers
set opt(prestop) 50; #time to notice nodes to stop their actions
set opt(stop) 60; #time to stop simulation
set opt(Rint) 4.3; #read interval
set opt(Wint) 4.3; #write interval
set opt(seed) 0;    #seed for randomness

#================================================#
# Simulation                                     #
#================================================#

#read input values if any
if { $argc > 3 } {
    set opt(ns) [expr [lindex $argv 0]]
    set opt(nr) [expr [lindex $argv 1]]
    set opt(t) [expr [lindex $argv 2]]
    set opt(Rint) [expr [lindex $argv 3]]
    set opt(Wint) [expr [lindex $argv 4]]
    set opt(seed) [expr [lindex $argv 5]]
}

#Create a simulator object
set ns [new Simulator]

#Open a trace file
set nf [open out.nam w]
$ns namtrace-all $nf

#Define a 'finish' procedure
proc finish {} {
        global ns nf
        $ns flush-trace
        close $nf
        #exec nam out.nam &
    exit 0
}

#Create the writer node
set node_(0) [$ns node]
set a_(0) [new Agent/Semifast]
$ns attach-agent $node_(0) $a_(0)
$a_(0) set nodeRole_ 3 #This is a writer
$a_(0) set S_ $opt(ns)
$a_(0) set t_ $opt(nt)
if {[expr ($opt(ns)/$opt(nt))-3] > 0} {
    $a_(0) setInterval $opt(Wint)
}
$a_(0) initialize
#$a_(0) set X_ 10
#$a_(0) set Y_ 0
#$a_(0) set Z_ 100


#Create the reader nodes
for { set k 1 } { $k <= $opt(nr) } { incr k } {
    set node_($k) [$ns node]
    set a_($k) [new Agent/Semifast]
    $ns attach-agent $node_($k) $a_($k)
    $a_($k) set nodeRole_ 2 #This is a reader
    $a_($k) set S_ $opt(ns)
    $a_($k) set t_ $opt(nt)
    if {[expr ($opt(ns)/$opt(nt))-3] > 0} {
	$a_($k) setInterval $opt(Rint)
    }
    $a_($k) initialize
 #   $node_($k) set X_ [expr $k*5]
 #   $node_($k) set Y_ 20
}

#Create the server nodes
for {set i [expr $opt(nr)+1]} {$i <= [expr $opt(ns)+$opt(nr)]} {incr i } {
    set node_($i) [$ns node]
    set a_($i) [new Agent/Semifast]
    $ns attach-agent $node_($i) $a_($i)
    $a_($i) set nodeRole_ 1 #This is a server
    $a_($i) set S_ $opt(ns)
    $a_($i) set R_ $opt(nr)
    $a_($i) set t_ $opt(nt)
    $a_($i) initialize
    if {[expr $i-$opt(nr)-1] < $opt(nt)} {
	$a_($i) set faulty_ 1
    } 
  #  $node_($i) set X_ [expr $i+5]
  #  $node_($i) set Y_ 10
}


#Connect the servers with the writer and the readers
for {set j 0} {$j <= $opt(nr)} { incr j } {
    for {set k [expr $opt(nr)+1]} {$k <= [expr $opt(ns)+$opt(nr)]} {incr k } {
	$ns duplex-link $node_($j) $node_($k) 1Mb 10ms DropTail
	$a_($j) connect-node $k
    }
}

#Schedule events
#$ns at 0.2 "$s0 write"
#$ns at 0.4 "$s1 read"
#$ns at 0.6 "$s0 write"
#$ns at 0.6 "$s1 read"
#$a_(2) connect $a_(9)
#$ns connect $a_(2) $a_(9)

#$a_(2) connect-node $a_(0)
#$a_(2) connect-node $a_(9)
#$a_(2) connect-node $a_(10)
for { set i 0 } { $i<[expr $opt(ns)+$opt(nr)+1] } { incr i } {
    $ns at $opt(prestop).0 "$a_($i) stop"
    $ns at $opt(stop).501 "$a_($i) terminate"
}

$ns at [expr $opt(stop)+1] "finish"

#Run the simulation
$ns run
