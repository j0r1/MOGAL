set terminal png
set output "beta.png"
p(x,b,s) = (1+b)/s*(1-x/s)**b
set xlabel "Genome index after sorting"
set ylabel "Genome selection probability distribution"
set size 1.0,0.8
plot [0:100] p(x,2.5,100) notitle
