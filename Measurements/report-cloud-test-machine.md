# Maket: test report

Maket 0.0.1, stage 0b. Machine: Intel(R) Xeon(R) Processor @ 2.10GHz, Linux. Date: 20 Nov 2026 12:13:48pm.

## 1. The test program

This program builds Maket's sources that hold no interface code (model, file formats, engines, notation) without the window, runs checks on them and writes this report. At stage 0b there is almost nothing to check yet: this section shows that the program builds, runs, can work with files and reports a result. From step 1a on, every step adds its own section.

| Check | Passed |
| --- | --- |
| The version reads as three numbers (`0.0.1`) | yes |
| The build stage reads as a step number and a letter (`0b`) | yes |
| A work folder can be made in the system's temporary folder and removed again | yes |
| A text file of 4114 bytes with accents and both kinds of line ending reads back byte for byte | yes |
| The report folder can be written | yes |

## 2. Test material

The files Joost puts in the repository's `Temp` folder are what the coming steps are tested on. This section lists what the program finds there; nothing is read yet. A missing file is not a failed check at this stage.

| File in `Temp` | Size |
| --- | --- |
| `doodsbed final v1c.txt` | 56.1 KB |
| `mixed_roll_example_01h.txt` | 17.2 KB |

Needed for step 1a (the bach text reader):

| File | Found |
| --- | --- |
| doodsbed final v1c | yes, as `doodsbed final v1c.txt` |
| mixed_roll_example_01h | yes, as `mixed_roll_example_01h.txt` |

All 5 checks passed.
