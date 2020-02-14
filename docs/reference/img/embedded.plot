set terminal png
set output "embedded.png"
set title 'Embedded p, (p-1) solution difference \~ h^p'
set xlabel "h"
set ylabel "difference between solutions"
set key top left
set logscale xy
set yrange [1e-17:1e-5]
set xrange [1e-6:1e-1]
f2(x) = c2*x**2
fit [0.0:0.1] f2(x) 'rk2' u 1:($2-$3) via c2
f3(x) = c3*x**3
fit [0.0:0.1] f3(x) 'rk3' u 1:($3-$2) via c3
f4(x) = c4*x**4
fit [0.004:0.1] f4(x) 'rk4' u 1:($3-$2) via c4
p \
'rk2' u 1:(sqrt(($3-$2)**2)) t "rk2(1)", \
f2(x) lt 0 lc 0 t "", \
'rk3' u 1:(sqrt(($3-$2)**2)) t "rk3(2)", \
f3(x) lt 0 lc 0 t "", \
'rk4' u 1:(sqrt(($3-$2)**2)) t "rk4(3)", \
f4(x) lt 0 lc 0 t ""
