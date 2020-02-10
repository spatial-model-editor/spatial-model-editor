set terminal png
set output "runtime.png"
set title 'Integrator Runtime \~ err^{-p}'
set ylabel "runtime"
set xlabel "relative error"
a = 1.6405957555239514e-01
set key top right
set logscale xy
set xrange [1e-11:1e-2]
set yrange [5:1e6]
f1(x) = c1*x**(-1.0)
fit [1e-9:1] f1(x) 'rk1' u (sqrt(($2-a)**2)/a):4 via c1
f2(x) = c2*x**(-1.0/2.0)
fit [1e-9:1] f2(x) 'rk2' u (sqrt(($2-a)**2)/a):4 via c2
f3(x) = c3*x**(-1.0/3.0)
fit [1e-9:1] f3(x) 'rk3' u (sqrt(($2-a)**2)/a):4 via c3
f4(x) = c4*x**(-1.0/4.0)
fit [1e-9:1] f4(x) 'rk4' u (sqrt(($2-a)**2)/a):4 via c4
p \
'rk1' u (sqrt(($2-a)**2)/a):4 t "rk1", \
f1(x) lc 0 lt 0 t "", \
'rk2' u (sqrt(($2-a)**2)/a):4 t "rk2", \
f2(x) lc 0 lt 0 t "", \
'rk3' u (sqrt(($2-a)**2)/a):4 t "rk3", \
f3(x) lc 0 lt 0 t "", \
'rk4' u (sqrt(($2-a)**2)/a):4 t "rk4", \
f4(x) lc 0 lt 0 t "", \
