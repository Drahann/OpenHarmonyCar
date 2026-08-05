import os

d = r'W:\paper\文档\assets'

# Define replacements: (file, old_string, new_string)
replacements = [
    # ch05.tex - 74.73pt overflow (worst)
    ('ch05.tex',
     r'\texttt{distributedDeviceManager}',
     r'\texttt{distributedDevice\allowbreak Manager}'),
    ('ch05.tex',
     r'\texttt{getAvailableDeviceListSync}',
     r'\texttt{getAvailable\allowbreak DeviceList\allowbreak Sync}'),
    ('ch05.tex',
     r'\texttt{ohos.permission.DISTRIBUTED\_DATASYNC}',
     r'\texttt{ohos.\allowbreak permission.\allowbreak DISTRIBUTED\_DATASYNC}'),
    ('ch05.tex',
     r'\texttt{udpm://239.255.76.67:7668?ttl=1}',
     r'\texttt{udpm://\allowbreak 239.255.76.67:\allowbreak 7668?ttl=1}'),
    ('ch05.tex',
     r'\texttt{FleetMissionService.init()}',
     r'\texttt{FleetMissionService.\allowbreak init()}'),

    # ch04.tex - 48.37pt, 43.53pt, 44.93pt overflows
    ('ch04.tex',
     r'\texttt{MapServer::saveZipedMap}',
     r'\texttt{MapServer::\allowbreak saveZipedMap}'),
    ('ch04.tex',
     r'\texttt{BCDCoverage::buildPathFromGridState}',
     r'\texttt{BCDCoverage::\allowbreak buildPathFromGridState}'),
    ('ch04.tex',
     r'\texttt{Lidar/lidar\_driver.cpp}',
     r'\texttt{Lidar/\allowbreak lidar\_driver.cpp}'),
    ('ch04.tex',
     r'\texttt{NAVI\_CreateFullPath}',
     r'\texttt{NAVI\allowbreak \_CreateFullPath}'),
    ('ch04.tex',
     r'\texttt{Navi/main.cpp}',
     r'\texttt{Navi/\allowbreak main.cpp}'),

    # ch03.tex - 59.14pt overflow
    ('ch03.tex',
     r'\texttt{decode\_one\_token}',
     r'\texttt{decode\allowbreak \_one\allowbreak \_token}'),
    ('ch03.tex',
     r'\texttt{@mindspore.jit}',
     r'\texttt{@mindspore.\allowbreak jit}'),

    # ch02b.tex - long paths
    ('ch02b.tex',
     r'\texttt{/base/startup/init/services/sandbox/system-sandbox64.json}',
     r'\texttt{/base/startup/init/\allowbreak services/sandbox/\allowbreak system-sandbox64.json}'),
    ('ch02b.tex',
     r'\texttt{/vendor/industio/purple\_pi\_oh/config.json}',
     r'\texttt{/vendor/industio/\allowbreak purple\_pi\_oh/\allowbreak config.json}'),
    ('ch02b.tex',
     r'\texttt{/out/rk3568/packages/phone/images}',
     r'\texttt{/out/rk3568/packages/\allowbreak phone/images}'),
    ('ch02b.tex',
     r'\texttt{aarch64-linux-gnu/libc/usr/lib}',
     r'\texttt{aarch64-linux-gnu/\allowbreak libc/usr/lib}'),
    ('ch02b.tex',
     r'\texttt{power-shell timeout -o 2147483647}',
     r'\texttt{power-shell timeout \allowbreak -o 2147483647}'),

    # ch06.tex
    ('ch06.tex',
     r'\texttt{range resolution height width metersPerPixel x0 y0}',
     r'\texttt{range resolution height width \allowbreak metersPerPixel x0 y0}'),
    ('ch06.tex',
     r'\texttt{MapService.parseMap}',
     r'\texttt{MapService.\allowbreak parseMap}'),

    # ch07.tex
    ('ch07.tex',
     r'\texttt{range resolution height width metersPerPixel x0 y0}',
     r'\texttt{range resolution height width \allowbreak metersPerPixel x0 y0}'),
]

for fname, old, new in replacements:
    fpath = os.path.join(d, fname)
    with open(fpath, 'r', encoding='utf-8') as f:
        content = f.read()
    count = content.count(old)
    if count > 0:
        content = content.replace(old, new)
        with open(fpath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Fixed {fname}: {count}x '{old[:40]}...'")
    else:
        print(f"NOT FOUND in {fname}: '{old[:40]}...'")

print("\nDone.")
