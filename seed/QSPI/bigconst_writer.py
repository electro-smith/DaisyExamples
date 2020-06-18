size = 1024 * 8
with open('test_const.h', 'w') as f:
    f.write('#include <stdint.h>\n')
    f.write('const uint32_t test_flash_buff[' + str(size) + '] =\n')
    f.write('{\n')
    for i in range(0, size):
        f.write(str(i) + ', ')
        if i % 8 == 0:
            f.write('\n')
    f.write('};\n')
