# L3 Timing Alignment Summary

This report compares ECC FastSTA `post_optimization` trace rows against Innovus evaluation `timing_paths.rpt` rows for the same ECC CTS result.

| Case | Matched | Unmatched | Mean abs arrival (ns) | Max abs arrival (ns) | Mean abs delay (ns) | Max abs delay (ns) | Classification |
|---|---:|---:|---:|---:|---:|---:|---|
| iwls2005__ac97_ctrl | 1250 | 0 | 0.0177945 | 0.0331971 | 0.00368668 | 0.0089866 | l3_aligned_under_50ps |
| iwls2005__des | 815 | 0 | 0.0207194 | 0.0438666 | 0.00380191 | 0.010602 | l3_aligned_under_50ps |
| iwls2005__mem_ctrl | 1300 | 0 | 0.00504613 | 0.019045 | 0.00235027 | 0.0071925 | l3_aligned_under_50ps |
| iwls2005__pci | 1043 | 87 | 0.0172198 | 0.0456501 | 0.00442665 | 0.0128234 | l3_aligned_under_50ps |
| iwls2005__sasc | 839 | 0 | 0.00397933 | 0.0112641 | 0.00149774 | 0.0048578 | l3_aligned_under_50ps |
| iwls2005__simple_spi | 900 | 0 | 0.00251773 | 0.00955043 | 0.00126629 | 0.005204 | l3_aligned_under_50ps |
| iwls2005__spi | 1100 | 0 | 0.00607116 | 0.0257097 | 0.00213619 | 0.0073203 | l3_aligned_under_50ps |
| iwls2005__ss_pcm | 723 | 0 | 0.00337067 | 0.0103275 | 0.00197266 | 0.0056323 | l3_aligned_under_50ps |
| iwls2005__systemcaes | 1100 | 0 | 0.00582736 | 0.0176055 | 0.00195666 | 0.0073211 | l3_aligned_under_50ps |
| iwls2005__systemcdes | 900 | 0 | 0.0044635 | 0.0170835 | 0.0016901 | 0.0045437 | l3_aligned_under_50ps |
| iwls2005__tv80 | 1076 | 0 | 0.00256368 | 0.0109198 | 0.00104307 | 0.0054995 | l3_aligned_under_50ps |
| iwls2005__usb_funct | 640 | 0 | 0.0127759 | 0.0427084 | 0.00366026 | 0.0145695 | l3_aligned_under_50ps |
| iwls2005__usb_phy | 870 | 0 | 0.00390294 | 0.0140347 | 0.00185603 | 0.0053331 | l3_aligned_under_50ps |
| iwls2005__vga_lcd | 2100 | 0 | 0.0264457 | 0.0783148 | 0.00459459 | 0.0237523 | cumulative_arrival_gap |
| iwls2005__wb_conmax | 1287 | 0 | 0.032898 | 0.0747031 | 0.0069403 | 0.0300619 | cumulative_arrival_gap |
| iwls2005__wb_dma | 1100 | 0 | 0.00598639 | 0.0171903 | 0.00180188 | 0.0065178 | l3_aligned_under_50ps |
| openroad__dynamic_node | 1495 | 5 | 0.00579926 | 0.0164184 | 0.00199348 | 0.0059704 | l3_aligned_under_50ps |
| openroad__ethmac | 1500 | 0 | 0.00952736 | 0.036733 | 0.00329443 | 0.0175887 | l3_aligned_under_50ps |
| openroad__fifo | 1300 | 0 | 0.00406004 | 0.020872 | 0.00146584 | 0.0061574 | l3_aligned_under_50ps |
| openroad__gcd | 291 | 0 | 0.00215766 | 0.00884814 | 0.00112611 | 0.0041447 | l3_aligned_under_50ps |
| openroad__jpeg | 1235 | 100 | 0.0208608 | 0.0409949 | 0.00463917 | 0.0103451 | l3_aligned_under_50ps |
| openroad__spi | 166 | 0 | 0.00245632 | 0.00706323 | 0.00116595 | 0.004146 | l3_aligned_under_50ps |
| openroad__uart | 707 | 0 | 0.00325483 | 0.0139623 | 0.00138309 | 0.0066716 | l3_aligned_under_50ps |
