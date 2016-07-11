looks like ecw uses something like:

$ kdu_compress -i /tmp/t.tiff -o /tmp/t.tiff.jp2 -record /tmp/t2.log Clevels=6 Cuse_eph=yes Corder=RPCL Creversible=yes Cprecincts="{128,128},{128,128},{128,128},{128,128},{128,128},{128,128},{64,64}" ORGgen_plt=yes -jp2_space sRGB Sprofile=PROFILE1 -no_info Qabs_ranges=10,12,11,11,12,11,11,12,11,11,12,11,11,12,11,11,12,11,11

