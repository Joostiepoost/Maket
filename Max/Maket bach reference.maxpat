{
    "patcher": {
        "fileversion": 1,
        "appversion": {
            "major": 9,
            "minor": 2,
            "revision": 0,
            "architecture": "x64",
            "modernui": 1
        },
        "classnamespace": "box",
        "rect": [
            120.0,
            140.0,
            900.0,
            480.0
        ],
        "boxes": [
            {
                "box": {
                    "id": "obj-c1",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        30.0,
                        20.0,
                        300.0,
                        24.0
                    ],
                    "text": "Maket bach reference",
                    "fontsize": 16.0,
                    "fontface": 1
                }
            },
            {
                "box": {
                    "id": "obj-c2",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        30.0,
                        48.0,
                        820.0,
                        62.0
                    ],
                    "text": "Plays a passage of Maket's timing test from bach.roll, inside Max, and sends every note it plays to the logging patch (\"Maket OSC log\" must be open). 1. Click read and choose \"timing passage dense.txt\" or \"timing passage long.txt\" in Maket's timing folder. 2. In Maket: Start a bach.roll take. 3. Click play here. 4. When it has played to its end: Finish the bach.roll take in Maket.",
                    "linecount": 4
                }
            },
            {
                "box": {
                    "id": "obj-1",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        30.0,
                        125.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "read"
                }
            },
            {
                "box": {
                    "id": "obj-2",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        85.0,
                        125.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "play"
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        140.0,
                        125.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "stop"
                }
            },
            {
                "box": {
                    "id": "obj-4",
                    "maxclass": "bach.roll",
                    "numinlets": 6,
                    "numoutlets": 8,
                    "patching_rect": [
                        30.0,
                        165.0,
                        820.0,
                        220.0
                    ],
                    "outlettype": [
                        "",
                        "",
                        "",
                        "",
                        "",
                        "",
                        "",
                        ""
                    ],
                    "out": "nnnnnntn",
                    "bwcompatibility": 80901,
                    "versionnumber": 80901
                }
            },
            {
                "box": {
                    "id": "obj-5",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        731.0,
                        400.0,
                        95.0,
                        22.0
                    ],
                    "text": "s maket_log_in"
                }
            },
            {
                "box": {
                    "id": "obj-c3",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        250.0,
                        401.0,
                        470.0,
                        33.0
                    ],
                    "text": "the playout outlet (the seventh), set to give text: every note as bach.roll plays it, at the moment it plays it",
                    "linecount": 2
                }
            }
        ],
        "lines": [
            {
                "patchline": {
                    "source": [
                        "obj-1",
                        0
                    ],
                    "destination": [
                        "obj-4",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-2",
                        0
                    ],
                    "destination": [
                        "obj-4",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-3",
                        0
                    ],
                    "destination": [
                        "obj-4",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-4",
                        6
                    ],
                    "destination": [
                        "obj-5",
                        0
                    ]
                }
            }
        ],
        "autosave": 0
    }
}