# L3 Timing Error Metrics

Metrics compare ECC L3 matched rows against Innovus timing rows. Delay uses `innovus_delay_ns` vs `ecc_combined_delay_ns`; arrival uses `innovus_arrival_ns` vs `ecc_arrival_ns`.

`WMAPE = sum(abs(error)) / sum(abs(Innovus))`; `sMAPE = mean(2*abs(error)/(abs(Innovus)+abs(ECC)))`. WMAPE is the preferred ratio for row delay because very small delay rows can make ordinary MAPE unstable.

## Overall

| eval | target | rows | WMAPE | sMAPE | R2 | RMSE ps | MSE ps^2 | MAE ps | p95 abs ps | max abs ps | <=10ps | <=50ps |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| detail_postroute_si | row_delay | 23737 | 6.594% | 21.988% | 0.945864 | 4.298 | 18.473 | 2.862 | 9.770 | 30.062 | 95.400% | 100.000% |
| detail_postroute_si | arrival | 23737 | 6.128% | 6.623% | 0.981903 | 16.852 | 283.975 | 11.161 | 34.372 | 78.315 | 61.583% | 96.811% |
| postcts_global_no_si | row_delay | 23821 | 6.205% | 21.671% | 0.952383 | 4.470 | 19.978 | 2.823 | 10.235 | 26.062 | 94.782% | 100.000% |
| postcts_global_no_si | arrival | 23821 | 5.390% | 5.747% | 0.984659 | 16.352 | 267.401 | 10.324 | 40.816 | 78.703 | 69.745% | 97.351% |

## Worst Case-Level WMAPE

### row_delay

| eval | case | rows | WMAPE | R2 | RMSE ps | MAE ps | max abs ps |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| postcts_global_no_si | `iwls2005__wb_conmax` | 1296 | 16.721% | 0.698098 | 9.693 | 8.302 | 26.062 |
| detail_postroute_si | `iwls2005__wb_conmax` | 1287 | 14.564% | 0.750728 | 8.834 | 6.940 | 30.062 |
| postcts_global_no_si | `iwls2005__vga_lcd` | 2100 | 10.078% | 0.909767 | 8.639 | 6.397 | 18.806 |
| detail_postroute_si | `iwls2005__des` | 815 | 9.838% | 0.894391 | 5.271 | 3.802 | 10.602 |
| detail_postroute_si | `iwls2005__ac97_ctrl` | 1250 | 9.015% | 0.883095 | 4.817 | 3.687 | 8.987 |
| detail_postroute_si | `iwls2005__pci` | 1043 | 8.902% | 0.673843 | 5.825 | 4.427 | 12.823 |
| detail_postroute_si | `openroad__jpeg` | 1235 | 8.353% | 0.843932 | 5.601 | 4.639 | 10.345 |
| detail_postroute_si | `iwls2005__usb_funct` | 640 | 7.812% | 0.941811 | 5.228 | 3.660 | 14.569 |
| postcts_global_no_si | `openroad__ethmac` | 1500 | 7.633% | 0.930826 | 6.823 | 4.463 | 19.939 |
| detail_postroute_si | `iwls2005__vga_lcd` | 2100 | 7.524% | 0.926843 | 6.694 | 4.595 | 23.752 |

### arrival

| eval | case | rows | WMAPE | R2 | RMSE ps | MAE ps | max abs ps |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| postcts_global_no_si | `iwls2005__wb_conmax` | 1296 | 18.740% | 0.804569 | 41.398 | 39.067 | 78.703 |
| detail_postroute_si | `iwls2005__wb_conmax` | 1287 | 16.402% | 0.840664 | 35.673 | 32.898 | 74.703 |
| detail_postroute_si | `iwls2005__des` | 815 | 10.715% | 0.942844 | 23.039 | 20.719 | 43.867 |
| detail_postroute_si | `iwls2005__pci` | 1043 | 10.343% | 0.928325 | 19.439 | 17.220 | 45.650 |
| detail_postroute_si | `iwls2005__ac97_ctrl` | 1250 | 10.225% | 0.933651 | 18.944 | 17.794 | 33.197 |
| detail_postroute_si | `openroad__jpeg` | 1235 | 9.386% | 0.941806 | 21.872 | 20.861 | 40.995 |
| detail_postroute_si | `iwls2005__usb_funct` | 640 | 8.213% | 0.943420 | 15.133 | 12.776 | 42.708 |
| postcts_global_no_si | `iwls2005__vga_lcd` | 2100 | 6.654% | 0.972732 | 33.563 | 27.911 | 75.138 |
| detail_postroute_si | `iwls2005__vga_lcd` | 2100 | 6.470% | 0.968699 | 33.456 | 26.446 | 78.315 |
| postcts_global_no_si | `iwls2005__systemcaes` | 1100 | 6.356% | 0.970930 | 11.833 | 11.175 | 19.862 |
