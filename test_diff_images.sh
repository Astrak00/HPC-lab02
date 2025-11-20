echo "Running tests..."

echo "\033[1;36mTest HSL\033[0m"
diff -q single_out_hsl.ppm out_hsl.ppm && echo  "\033[1;32m✓ PASSED\033[0m" || echo  "\033[1;31m✗ FAILED\033[0m"

echo "\033[1;36mTest Y'UV\033[0m"
diff -q single_out_yuv.ppm out_yuv.ppm && echo  "\033[1;32m✓ PASSED\033[0m" || echo  "\033[1;31m✗ FAILED\033[0m"

echo "\033[1;36mSimple\033[0m"
diff -q single_out.pgm out.pgm && echo  "\033[1;32m✓ PASSED\033[0m" || echo  "\033[1;31m✗ FAILED\033[0m"