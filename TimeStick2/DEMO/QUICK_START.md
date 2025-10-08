# Quick Start Guide for Demo

## For Your Coworker Tomorrow

### TL;DR
1. Run `./demo_1_compile.sh` - Shows it compiles
2. Run `sudo ./demo_2_standalone.sh` - Shows hardware works
3. Done! The fix is proven.

---

## Running the Demos

**Demo scripts are in TimeStick2/DEMO directory**:

```bash
# Option A: Run from the DEMO directory (recommended)
cd /path/to/TimeStick/TimeStick2/DEMO
./demo_1_compile.sh
sudo ./demo_2_standalone.sh

# Option B: Run from anywhere with full path
/path/to/TimeStick/TimeStick2/DEMO/demo_1_compile.sh
sudo /path/to/TimeStick/TimeStick2/DEMO/demo_2_standalone.sh

# Option C: Add DEMO directory to PATH (for convenience)
export PATH="$PATH:/path/to/TimeStick/TimeStick2/DEMO"
demo_1_compile.sh
sudo demo_2_standalone.sh
```

Scripts automatically find the driver directory (../DRV) relative to their location.

---

## The 2-Minute Demo

### Step 1: Show the Problem (30 seconds)
Say: *"The driver failed to compile on kernel 6.16+ because the build system changed"*

### Step 2: Show the Fix (30 seconds)
```bash
# Open Makefile and scroll to show ccflags-y lines
grep "ccflags-y" ../DRV/Makefile | head -5
# Or from repo root:
grep "ccflags-y" TimeStick2/DRV/Makefile | head -5
```

Say: *"We added these 15 lines to make it work with modern kernels"*

### Step 3: Prove It Compiles (30 seconds)
```bash
./demo_1_compile.sh
```

Say: *"Now it compiles successfully. Watch for the green checkmarks."*

### Step 4: Prove Hardware Works (30 seconds)
```bash
sudo ./demo_2_standalone.sh
```

Say: *"And the hardware is fully functional. See the PTP hardware clock and timestamping capabilities."*

**Done!** You've proven the fix works.

---

## What Each Demo Shows

| Demo | What It Proves | Required Hardware |
|------|----------------|-------------------|
| Demo 1 | Driver compiles on kernel 6.16+ | None |
| Demo 2 | PTP hardware works correctly | TimeStick plugged in |
| Demo 3 | Full PTP sync (optional) | TimeStick + Network + PTP master |

**For proving the fix works, you only need Demo 1 + Demo 2.**

---

## Key Points to Mention

1. **The Problem**: Kernel 6.16+ requires `ccflags-y` instead of deprecated `EXTRA_CFLAGS`
2. **The Solution**: Added 15 lines to Makefile (one `ccflags-y` for each compile flag)
3. **The Result**: Driver compiles and works perfectly on modern kernels
4. **The Proof**: 
   - `/dev/ptp0` exists ✓
   - Hardware timestamping available ✓
   - All PTP modes supported ✓

---

## Expected Output Highlights

### Demo 1 Success:
```
✓ Found 15 ccflags-y declarations in Makefile
✓ No 'incompatible pointer type' errors
✓ All files created successfully
╔════════════════════════════════════════╗
║    ✓ COMPILATION SUCCESSFUL            ║
╚════════════════════════════════════════╝
```

### Demo 2 Success:
```
✓ Found USB device: AX88279
✓ Driver module loaded (ax_usb_nic)
✓ PTP hardware clock found! (/dev/ptp0)

Hardware Timestamping:
  ✓ Hardware TX timestamping: SUPPORTED
  ✓ Hardware RX timestamping: SUPPORTED
  ✓ PTPv2 (IEEE 1588-2008): SUPPORTED
  
✓✓✓ ALL TESTS PASSED! ✓✓✓
```

---

## Troubleshooting

### "No such file or directory"
**Problem**: Running from wrong location  
**Solution**: Scripts now work from anywhere! Just use full path or cd to driver dir

### "Module not found"
**Problem**: Haven't compiled yet  
**Solution**: Run `demo_1_compile.sh` first

### "Must be run as root"
**Problem**: Demo 2 and 3 need root  
**Solution**: Use `sudo`

---

## Files You'll Use

- `demo_1_compile.sh` - Compilation test (no sudo needed)
- `demo_2_standalone.sh` - Hardware test (needs sudo)
- `demo_3_ptp_sync.sh` - Full PTP sync (optional, needs sudo + network)
- `DEMO_README.md` - Detailed documentation
- `Makefile` - Contains the fix (show the ccflags-y lines)

---

## Presentation Flow

1. **Context** (1 min): "Driver broke on new kernel"
2. **Demo 1** (1 min): Run compilation test
3. **Show Fix** (30 sec): Open Makefile, show ccflags-y
4. **Demo 2** (1 min): Run hardware test
5. **Explain** (1 min): Why it matters (hardware timestamping)
6. **Q&A** (time permitting)

**Total: 4-5 minutes**

---

## One-Line Explanation

*"We added modern kernel build system flags (ccflags-y) so the driver can compile on kernel 6.16+ and properly initialize PTP hardware timestamping."*

---

## Success Metrics

After your demo, your coworker should understand:
- ✅ What broke (compilation on kernel 6.16+)
- ✅ Why it broke (build system change)
- ✅ How we fixed it (added ccflags-y)
- ✅ That it works (hardware timestamping functional)

Good luck! 🎉
