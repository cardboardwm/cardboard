#!/bin/sh

for f in $(find . -iname '*.h'); do
    placeholder="$(dirname "$f")/__$(basename "$f")"
    echo '#ifdef __cplusplus
extern "C" {
#endif' >> "$placeholder"
    cat "$f" >> "$placeholder"
    echo '#ifdef __cplusplus
}
#endif' >> "$placeholder"
    mv "$placeholder" "$f"
	sed -i 's/#include <wlr/#include <wlr_cpp/g' "$f"
done

pattern='([a-zA-Z_]+)\[static [0-9]\]'
ag -l "$pattern" | \
    xargs sed -E -i "s/$pattern/"'*\1/g'
