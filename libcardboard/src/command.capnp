@0xb5ec1a46139ed528;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("generated");

struct Command {
    enum Commands {
        quit @0;
        focus @1;
        bind @2;
        exec @3;
    }

    enum FocusDirection {
        left @0;
        right @1;
    }

    struct Bind{
        modifiers @0 :List(Text);
        key @1 :Text;
        command @2 :Command;
    }

    command @0 :Commands;
    arguments :union {
        focusDirection @1 :FocusDirection;
        execCommand @2 :List(Text);
        bind @3 :Bind;
    }
}