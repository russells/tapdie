
Element["" "SMD CR2032 holder" "B?" "" 393701 196850 43307 -11811 0 160 ""]
(

	# If the cut-out circle has to move, leave these pads alone.
	Pad[0 -3150 0 3150 12598 2000 13598 "" "1" "square"]
	Pad[122441 -3150 122441 3150 12598 2000 13598 "" "2" "square"]
	ElementLine [-7874 -11024 7874 -11024 1000]
	ElementLine [7874 -11024 7874 11024 1000]
	ElementLine [7874 11024 -7874 11024 1000]
	ElementLine [-7874 11024 -7874 -11024 1000]

	# If the cut out circle has to move, move all of these lines
	# with it.
	ElementLine [114567 -11024 130315 -11024 1000]
	ElementLine [130315 -11024 130315 11024 1000]
	ElementLine [130315 11024 114567 11024 1000]
	ElementLine [114567 11024 114567 -11024 1000]
	ElementLine [40551 -37402 89370 -37402 1000]
	ElementLine [40551 37402 44488 37402 1000]
	ElementLine [89370 37402 85433 37402 1000]
	ElementLine [44488 37402 50000 31890 1000]
	ElementLine [85433 37402 79921 31890 1000]
	ElementLine [79921 31890 50000 31890 1000]
	ElementLine [23622 0 31496 0 1000]
	ElementLine [27559 -3937 27559 3937 1000]
	ElementLine [106299 0 98425 0 1000]

	# These arcs were created in pcb, and edited to create
	# the arcs below.
	#
	# ElementArc [64961 0 44882 44882 270 90 1000]
	# ElementArc [64961 0 44882 44882 180 90 1000]
	# ElementArc [64961 0 44882 44882 90 90 1000]
	# ElementArc [64961 0 44882 44882 0 90 1000]

	ElementArc [64961 0 44882 44882 0 -56.5 1000]
	ElementArc [64961 0 44882 44882 180 56.5 1000]
	ElementArc [64961 0 44882 44882 180 -56.5 1000]
	ElementArc [64961 0 44882 44882 0 56.5 1000]

	)
