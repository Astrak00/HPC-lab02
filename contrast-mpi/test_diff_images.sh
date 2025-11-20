echo "Running tests..."

echo "\033[1;36mTest HSL\033[0m"
diff -q out_hsl_single.ppm out_hsl.ppm && echo  "\033[1;32m✓ PASSED\033[0m" || echo  "\033[1;31m✗ FAILED\033[0m"

echo "\033[1;36mTest Y'UV\033[0m"
diff -q out_yuv_single.ppm out_yuv.ppm && echo  "\033[1;32m✓ PASSED\033[0m" || echo  "\033[1;31m✗ FAILED\033[0m"

echo "\033[1;36mSimple\033[0m"
diff -q out_single.pgm out.pgm && echo  "\033[1;32m✓ PASSED\033[0m" || echo  "\033[1;31m✗ FAILED\033[0m"