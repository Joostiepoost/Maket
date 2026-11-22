# Maket timing tests (stage 1d)

Every line is one take. Times in milliseconds: how far the messages arrived from the time they were due, after the two clocks were laid on each other at the middle message.

| When | Passage | Played by | Condition | Arrived | Out of order | Typical (ms) | Half within | 95 in 100 within | 99 in 100 within | Farthest | Left Maket at the latest |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 22 Nov 19:29:50 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.16 | 0.05 | 0.42 | 0.54 | 2.71 | 0.69 |
| 22 Nov 19:31:38 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.23 | 0.08 | 0.33 | 0.53 | 8.20 | 0.66 |
| 22 Nov 19:33:33 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 1024, Signal vector 1024, no CPU limit | 2806 of 2806 | 0 | 0.14 | 0.07 | 0.36 | 0.48 | 0.83 | 0.63 |
| 22 Nov 19:35:30 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.17 | 0.05 | 0.42 | 0.61 | 3.13 | 0.70 |
| 22 Nov 19:36:24 | dense | Maket (with the ePlayer) | test1 - overdrive, IO vector 512, Signal vector 64, no CPU limit | 2806 of 2806 | 0 | 0.17 | 0.06 | 0.38 | 0.56 | 3.66 | 0.69 |
