# Maket timing tests (stage 1d)

Every line is one take. Times in milliseconds: how far the messages arrived from the time they were due, after the two clocks were laid on each other at the middle message.

| When | Passage | Played by | Condition | Arrived | Out of order | Typical (ms) | Half within | 95 in 100 within | 99 in 100 within | Farthest | Left Maket at the latest |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 22 Nov 19:29:50 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.16 | 0.05 | 0.42 | 0.54 | 2.71 | 0.69 |
| 22 Nov 19:31:38 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.23 | 0.08 | 0.33 | 0.53 | 8.20 | 0.66 |
| 22 Nov 19:33:33 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 1024, Signal vector 1024, no CPU limit | 2806 of 2806 | 0 | 0.14 | 0.07 | 0.36 | 0.48 | 0.83 | 0.63 |
| 22 Nov 19:35:30 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.17 | 0.05 | 0.42 | 0.61 | 3.13 | 0.70 |
| 22 Nov 19:36:24 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.17 | 0.06 | 0.38 | 0.56 | 3.66 | 0.69 |
| 22 Nov 19:46:44 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.15 | 0.07 | 0.31 | 0.42 | 3.24 | 0.71 |
| 22 Nov 19:46:44 | dense | bach.roll, while Maket played | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 1012 of 1403 | 0 | 0.18 | 0.03 | 0.17 | 0.21 | 5.28 |  |
| 22 Nov 19:48:18 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.22 | 0.07 | 0.33 | 0.45 | 6.91 | 0.78 |
| 22 Nov 19:48:18 | dense | bach.roll, while Maket played | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 881 of 1403 | 0 | 0.29 | 0.03 | 0.16 | 0.20 | 8.57 |  |
| 22 Nov 19:50:10 | dense | Maket (with the ePlayer) | test4 - overdrive off | 2806 of 2806 | 0 | 3.94 | 0.72 | 8.73 | 18.91 | 29.86 | 0.68 |
| 22 Nov 19:52:07 | dense | Maket (with the ePlayer) | test5 - overdrive on - scheduler in audio interrupt on | 2806 of 2806 | 0 | 1.43 | 1.18 | 2.48 | 2.81 | 3.17 | 0.68 |
| 22 Nov 19:58:14 | dense | Maket (with the ePlayer) | test6b - overdrive on - io vector 512 - signal vector 128 - max kept busy | 2806 of 2806 | 0 | 0.13 | 0.05 | 0.35 | 0.52 | 0.69 | 0.72 |
| 22 Nov 20:00:39 | long | Maket (with the ePlayer) | test7 - overdrive on - io vector 512 - signal vector 128 - long passage | 6000 of 6000 | 0 | 17.55 | 0.04 | 38.88 | 96.71 | 99.36 | 99.42 |
| 22 Nov 20:07:11 | long | Maket (with the ePlayer) | test7b - overdrive on - io vector 512 - signal vector 128 - long passage (without touching the laptop) | 6000 of 6000 | 0 | 0.06 | 0.03 | 0.12 | 0.18 | 0.42 | 0.39 |
| 22 Nov 20:43:55 | long | Maket (with the ePlayer) | test7d - overdrive on - io vector 512 - signal vector 128 - long passage (without touching the laptop - after cooling down - no visual activity in max) | 6000 of 6000 | 0 | 0.05 | 0.03 | 0.12 | 0.16 | 0.64 | 0.28 |
| 22 Nov 21:34:11 | long | Maket (with the ePlayer) | Test8 - overdrive on - scheduler in audio thread off - io vector 512 - signal vector 128 | 6000 of 6000 | 0 | 20.87 | 0.06 | 69.78 | 97.40 | 103.10 | 103.35 |
| 22 Nov 21:44:01 | long | Maket (with the ePlayer) | Test 8b - conditions same as 8a - Maket kept in focus | 6000 of 6000 | 0 | 0.07 | 0.05 | 0.14 | 0.18 | 1.12 | 0.28 |
| 22 Nov 22:22:12 | long | Maket (with the ePlayer) | Test9 - usual settings - bringing other app in front; | 6000 of 6000 | 0 | 0.26 | 0.05 | 0.16 | 0.22 | 12.99 | 0.32 |
