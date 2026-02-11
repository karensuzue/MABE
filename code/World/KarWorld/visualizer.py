"""
This is a visualization tool that can play frames 
automatically as an animation or be controlled with the keyboard
Keyboard controls:
- Space: pause / play
- Arrow: backward / forward
- r : restart
- q: quit
"""

import argparse
import curses
import time
import re
from pathlib import Path
from typing import List


def split_frames(text: str, expected_rows) -> List[List[str]]:
    """
    Splits text into a list of frames :)
    Does row count checking. 
    Too lazy to implement column count checking (it's probably not needed). 

    Expected frame format:
    **Some Header**
    (column indices line)
    +---------+
    0 | ... |
    ...
    39 | ... |
    +---------+
    """
    # Header matching (**World Step: 0** or **Initialize World**)
    header_re = re.compile(r"^\*\*(.+?)\*\*\s*$")

    # Grid row matching (0 | .  .  . ... |)
    row_re = re.compile(r"^\s*(\d+)\s*\|\s*(.*?)\s*\|\s*$")

    # Grid border matching (+----------+)
    border_re = re.compile(r"^\s*\+[-+]+\+\s*$")

    lines: List[str] = text.splitlines()
    frames: List[List[str]] = []

    i = 0
    while (i < len(lines)):
        # Find a header that starts a frame
        header_match = header_re.match(lines[i])
        if not header_match:
            i += 1
            continue

        # Make sure we can safely read column indices + top border:
        if i + 2 >= len(lines): break
        
        # Hard-check
        col_indices = lines[i + 1]
        top_border = lines[i + 2]
        if not border_re.match(top_border):
            raise ValueError(
                f"Expected a border line after header at line {i}, "
                "but got something else..."
            )
        
        # Store all lines associated with current frame
        frame: List[str] = [] 
        frame.append(lines[i]) # keep header line
        frame.append(col_indices) # keep column indices
        frame.append(top_border) # keep top border
        i += 3 # skip to grid rows

        # Consume row lines until we hit the bottom border
        row_count = 0
        while (i < len(lines) and not border_re.match(lines[i])):
            if row_re.match(lines[i]):
                frame.append(lines[i])
                row_count += 1
            i += 1

        if i >= len(lines):
            break

        frame.append(lines[i]) # keep bottom border    
        i += 1

        if row_count == expected_rows:
            frames.append(frame)
        else:
            print("Warning! A frame does not contain the expected number of rows!")
        
    return frames

def player(stdscr, frames: List[List[str]], fps: float) -> None:
    # Delay is 1/fps, with safety guard to avoid zero division
    delay = 1.0 / max(1e-6, fps) 

    idx = 0
    paused = False
    last_tick = time.time()
    while True:
        now = time.time() # current time in seconds
        # --- INPUT ---
        ch = stdscr.getch() # this returns ASCII number
        if ch != -1: 
            if ch in (ord("q"), ord("Q")): # quit
                return
            elif ch == ord(" "): # pause
                paused = not paused
            elif ch == curses.KEY_RIGHT: # move forward
                idx = (idx + 1) % len(frames)
            elif ch == curses.KEY_LEFT: # move backward
                idx = (idx - 1 + len(frames)) % len(frames)
            elif ch in (ord("r"), ord('R')):
                idx = 0
        
        # --- ADVANCE TO NEXT FRAME ---
        # if paused, and enough time has passed since the last frame
        if not paused and (now - last_tick) >= delay:
            idx = (idx + 1) % len(frames)
            last_tick = now
        
        # --- RENDER ---
        stdscr.erase() 
        h, w = stdscr.getmaxyx() # terminal size

        frame_lines = frames[idx] # grab lines from current frame
        # Draw as many frame lines as fit on screen
        for row_i in range(min(h, len(frame_lines))):
            line = frame_lines[row_i]
            # draws at most w-1 characters from line at row "row_i", column 0
            stdscr.addnstr(row_i, 0, line, w - 1)

        stdscr.refresh()
        time.sleep(0.005)

def main():
    ap = argparse.ArgumentParser(description="Play ASCII world frames from a text file with keyboard controls.")
    ap.add_argument("--path", type=Path, required=True, help="Path to the text file containing frames.")
    ap.add_argument("--row_count", type=int, required=True, help="How many rows are in this world")
    ap.add_argument("--col_count", type=int, help="How many columns are in this world?")
    ap.add_argument("--fps", default=12.0, type=float, help="FPS.")
    args = ap.parse_args()

    text = args.path.read_text()
    frames = split_frames(text, args.row_count) # list of frames
    
    if not frames:
        print("No frames found!")

    # Initialize curses object
    stdscr = curses.initscr()
    # Do not echo keys back to client
    curses.noecho()
    # Do not wait for Enter key to be pressed
    curses.cbreak() 
    #turn off blink cursors
    curses.curs_set(False)
    # getch pauses program until it gets a key, nodelay turns that off for animation to work
    stdscr.nodelay(True)
    # Enable the keypad
    stdscr.keypad(True)

    caught_except = ""
    try:
        player(stdscr, frames, args.fps)
    except Exception as err:
        # Apparently just printing from here will not work
        # because program is still stuck in curses
        caught_except = str(err)

    # Reverse curses terminal settings
    curses.nocbreak()
    curses.echo()
    curses.curs_set(True)
    stdscr.keypad(False)
    
    # Terminate
    curses.endwin()

    if "" != caught_except:
        print("Error(s) caught: " + caught_except)

if __name__ == "__main__":
    main()