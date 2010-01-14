hyacc, hyaccpar and hyaccmanpage are the three files
needed to run Hyacc. To use Hyacc, put your grammar 
file in the same folder as these three files.

"hyacc -h" will show help message.
"hyacc -m" will show manual.

One example calc.y (an infix notation calculator) is provided.
"hyacc calc.y" will produce y.tab.c. 
"gcc y.tab.c -o calc" will produce the executable calc.

