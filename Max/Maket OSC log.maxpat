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
            80.0,
            100.0,
            930.0,
            520.0
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
                    "text": "Maket OSC log",
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
                        560.0,
                        47.0
                    ],
                    "text": "Notes when each OSC message from Maket arrives in Max. Every message gets a number and the time on the computer's clock, in milliseconds since the last reset, and is kept in the coll below.",
                    "linecount": 3
                }
            },
            {
                "box": {
                    "id": "obj-1",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        30.0,
                        120.0,
                        110.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "udpreceive 46002"
                }
            },
            {
                "box": {
                    "id": "obj-c3",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        150.0,
                        121.0,
                        440.0,
                        20.0
                    ],
                    "text": "the port: the same number as \"Logging patch port\" in Maket's preferences"
                }
            },
            {
                "box": {
                    "id": "obj-2",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 3,
                    "patching_rect": [
                        30.0,
                        160.0,
                        170.0,
                        22.0
                    ],
                    "outlettype": [
                        "",
                        "bang",
                        "bang"
                    ],
                    "text": "t l b b"
                }
            },
            {
                "box": {
                    "id": "obj-c4",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        210.0,
                        161.0,
                        400.0,
                        20.0
                    ],
                    "text": "right to left: first the time, then the number, then the message itself"
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        181.0,
                        200.0,
                        60.0,
                        22.0
                    ],
                    "outlettype": [
                        "float"
                    ],
                    "text": "cpuclock"
                }
            },
            {
                "box": {
                    "id": "obj-4",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        181.0,
                        232.0,
                        60.0,
                        22.0
                    ],
                    "outlettype": [
                        "float"
                    ],
                    "text": "- 0."
                }
            },
            {
                "box": {
                    "id": "obj-c5",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        250.0,
                        233.0,
                        220.0,
                        20.0
                    ],
                    "text": "milliseconds since the last reset"
                }
            },
            {
                "box": {
                    "id": "obj-5",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        105.0,
                        200.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        "int"
                    ],
                    "text": "int"
                }
            },
            {
                "box": {
                    "id": "obj-6",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        124.0,
                        232.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        "int"
                    ],
                    "text": "+ 1"
                }
            },
            {
                "box": {
                    "id": "obj-7",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        105.0,
                        272.0,
                        95.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "pack 0 0."
                }
            },
            {
                "box": {
                    "id": "obj-8",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        105.0,
                        304.0,
                        75.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "prepend set"
                }
            },
            {
                "box": {
                    "id": "obj-9",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        30.0,
                        344.0,
                        94.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "prepend 0"
                }
            },
            {
                "box": {
                    "id": "obj-c6",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        135.0,
                        345.0,
                        470.0,
                        20.0
                    ],
                    "text": "number, time, message: for example  12 3410.52 noteon voice1 60. 100"
                }
            },
            {
                "box": {
                    "id": "obj-10",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 4,
                    "patching_rect": [
                        30.0,
                        420.0,
                        120.0,
                        22.0
                    ],
                    "outlettype": [
                        "",
                        "",
                        "",
                        ""
                    ],
                    "text": "coll maket_osc_log"
                }
            },
            {
                "box": {
                    "id": "obj-c7",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        160.0,
                        421.0,
                        200.0,
                        20.0
                    ],
                    "text": "double-click to see the lines"
                }
            },
            {
                "box": {
                    "id": "obj-11",
                    "maxclass": "number",
                    "numinlets": 1,
                    "numoutlets": 2,
                    "patching_rect": [
                        260.0,
                        272.0,
                        60.0,
                        22.0
                    ],
                    "outlettype": [
                        "",
                        "bang"
                    ],
                    "parameter_enable": 0
                }
            },
            {
                "box": {
                    "id": "obj-c8",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        330.0,
                        273.0,
                        260.0,
                        20.0
                    ],
                    "text": "the number of the last message"
                }
            },
            {
                "box": {
                    "id": "obj-12",
                    "maxclass": "toggle",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        330.0,
                        380.0,
                        22.0,
                        22.0
                    ],
                    "outlettype": [
                        "int"
                    ],
                    "parameter_enable": 0
                }
            },
            {
                "box": {
                    "id": "obj-13",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        330.0,
                        420.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "gate"
                }
            },
            {
                "box": {
                    "id": "obj-14",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        330.0,
                        452.0,
                        75.0,
                        22.0
                    ],
                    "text": "print Maket"
                }
            },
            {
                "box": {
                    "id": "obj-c9",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        360.0,
                        381.0,
                        340.0,
                        20.0
                    ],
                    "text": "switch on to see every message in the Max window as well"
                }
            },
            {
                "box": {
                    "id": "obj-15",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        470.0,
                        120.0,
                        60.0,
                        22.0
                    ],
                    "outlettype": [
                        "bang"
                    ],
                    "text": "loadbang"
                }
            },
            {
                "box": {
                    "id": "obj-16",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        545.0,
                        120.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "reset"
                }
            },
            {
                "box": {
                    "id": "obj-c10",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        595.0,
                        121.0,
                        300.0,
                        33.0
                    ],
                    "text": "click before a test: empties the log, and starts the clock at 0 and the numbers at 1",
                    "linecount": 2
                }
            },
            {
                "box": {
                    "id": "obj-17",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 3,
                    "patching_rect": [
                        470.0,
                        160.0,
                        115.0,
                        22.0
                    ],
                    "outlettype": [
                        "bang",
                        "bang",
                        "bang"
                    ],
                    "text": "t b b b"
                }
            },
            {
                "box": {
                    "id": "obj-18",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        566.0,
                        200.0,
                        60.0,
                        22.0
                    ],
                    "outlettype": [
                        "float"
                    ],
                    "text": "cpuclock"
                }
            },
            {
                "box": {
                    "id": "obj-19",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        518.0,
                        200.0,
                        30.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "1"
                }
            },
            {
                "box": {
                    "id": "obj-20",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        470.0,
                        380.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "clear"
                }
            },
            {
                "box": {
                    "id": "obj-21",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        30.0,
                        380.0,
                        40.0,
                        22.0
                    ],
                    "outlettype": [
                        ""
                    ],
                    "text": "write"
                }
            },
            {
                "box": {
                    "id": "obj-c11",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        80.0,
                        381.0,
                        240.0,
                        33.0
                    ],
                    "text": "click after a test: saves the log as a text file (Max asks where)",
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
                        "obj-2",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-2",
                        2
                    ],
                    "destination": [
                        "obj-3",
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
                        0
                    ],
                    "destination": [
                        "obj-7",
                        1
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-2",
                        1
                    ],
                    "destination": [
                        "obj-5",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-5",
                        0
                    ],
                    "destination": [
                        "obj-6",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-6",
                        0
                    ],
                    "destination": [
                        "obj-5",
                        1
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-5",
                        0
                    ],
                    "destination": [
                        "obj-7",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-5",
                        0
                    ],
                    "destination": [
                        "obj-11",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-7",
                        0
                    ],
                    "destination": [
                        "obj-8",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-8",
                        0
                    ],
                    "destination": [
                        "obj-9",
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
                        "obj-9",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-9",
                        0
                    ],
                    "destination": [
                        "obj-10",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-9",
                        0
                    ],
                    "destination": [
                        "obj-13",
                        1
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-12",
                        0
                    ],
                    "destination": [
                        "obj-13",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-13",
                        0
                    ],
                    "destination": [
                        "obj-14",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-15",
                        0
                    ],
                    "destination": [
                        "obj-17",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-16",
                        0
                    ],
                    "destination": [
                        "obj-17",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-17",
                        2
                    ],
                    "destination": [
                        "obj-18",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-18",
                        0
                    ],
                    "destination": [
                        "obj-4",
                        1
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-17",
                        1
                    ],
                    "destination": [
                        "obj-19",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-19",
                        0
                    ],
                    "destination": [
                        "obj-5",
                        1
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-17",
                        0
                    ],
                    "destination": [
                        "obj-20",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-20",
                        0
                    ],
                    "destination": [
                        "obj-10",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-21",
                        0
                    ],
                    "destination": [
                        "obj-10",
                        0
                    ]
                }
            }
        ],
        "autosave": 0
    }
}