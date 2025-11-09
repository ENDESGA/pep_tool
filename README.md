<p align="center">
  <img width="725" height="383" alt="pep_logo" src="https://github.com/user-attachments/assets/e619c440-4b0d-40c6-9274-86ee163162ad" />
</p>

```
usage: pep_tool [channel_bits:1/2/4/8] <input> [<output>]

converts: image <-> pep | default: .image->.pep, .pep->.png

input formats:
  pep, jpg, bmp, tga, png, gif (first frame), pnm, pic

output formats:
  pep, jpg, bmp, tga, png

examples:
  pep_tool channel_bits:2 image.png (makes image.pep)
  pep_tool image.pep output.jpg (makes output.jpg)
  pep_tool image.bmp (makes image.pep with 8bits/channel)
```

This uses the stb_image libraries to read/write most image formats to convert to ***pep*** format.

Please support Sean Barrett's work: https://github.com/nothings/stb
