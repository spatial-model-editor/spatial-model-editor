# data from ./benchmark/pixel [1,2,3,4] bru 40 0 0 100 > rk[1,2,3,4]
set terminal png
set output "convergence.png"
set title 'Integrator Convergence \~ h^p'
set xlabel "h"
set ylabel "relative error"
a = 1.6405957555239514e-01
set key top left
set logscale xy
set yrange [1e-13:1e-1]
set xrange [1e-6:1e-1]
f1(x) = a + c1*x
fit [0:0.01] f1(x) 'rk1' u 1:2 via c1
f2(x) = a + c2*x**2
fit [0:0.01] f2(x) 'rk2' u 1:2 via c2
f3(x) = a + c3*x**3
fit [0:0.01] f3(x) 'rk3' u 1:2 via c3
f4(x) = a + c4*x**4
fit [0:0.01] f4(x) 'rk4' u 1:2 via c4
p \
'rk1' u 1:(sqrt(($2-a)**2)/a) t "rk1", \
sqrt(c1**2)*x/a lt 0 lc 0 t "", \
'rk2' u 1:(sqrt(($2-a)**2)/a) t "rk2", \
sqrt(c2**2)*x**2/a lt 0 lc 0 t "", \
'rk3' u 1:(sqrt(($2-a)**2)/a) t "rk3", \
sqrt(c3**2)*x**3/a lt 0 lc 0 t "", \
'rk4' u 1:(sqrt(($2-a)**2)/a) t "rk4", \
sqrt(c4**2)*x**4/a lt 0 lc 0 t ""
