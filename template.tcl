#Create a simulator object
set ns [new Simulator]

#Define different colors for data flows
#$ns color 1 Blue
#$ns color 2 Red

#Open the nam trace file
set nf [open out.nam w]
$ns namtrace-all $nf

#Define a 'finish' procedure
proc finish {} {
        global ns nf
        $ns flush-trace
	#Close the trace file
        close $nf
	#Execute nam on the trace file
        # exec nam out.nam &
        exit 0
}

--END

for {set i 0} {$i < 4} {incr i} {
    set n($i) [$ns node]
}

set d0 [new Agent/DuSM]
set d1 [new Agent/DuSM]
set d2 [new Agent/DuSM]
set d3 [new Agent/DuSM]

$d0 init-gc
$ns attach-agent $n(0) $d0
$d0 addr 0
$ns attach-agent $n(1) $d1
$d1 addr 1
$ns attach-agent $n(2) $d2
$d2 addr 2
$ns attach-agent $n(3) $d3
$d3 addr 3

#Create links between the nodes
$ns duplex-link $n(0) $n(1) 1Mb 10ms DropTail
$ns duplex-link $n(0) $n(2) 1Mb 10ms DropTail
$ns duplex-link $n(0) $n(3) 1Mb 10ms DropTail

#Call the finish procedure after 5 seconds of simulation time
$ns at 0.0 "$d2 post 0 1000"
$ns at 5.0 "finish"

#Run the simulation
$ns run
