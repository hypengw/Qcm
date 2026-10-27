#include "../../app/src/message/include/Qcm/message/comment_reaction.hpp"
#include <cassert>

int main() {
    qcm::comments::Reaction reaction;
    assert(! reaction.begin(1));
    reaction.read(false, 0, 2);
    assert(reaction.liked == false && reaction.count == 0);
    assert(reaction.begin(3));
    assert(! reaction.begin(4));
    reaction.read(true, 99, 4);
    assert(reaction.liked == false && reaction.count == 0);
    reaction.complete(true, {}, 5);
    assert(reaction.liked == true && reaction.count == 1 && ! reaction.pending);
    reaction.read(false, 0, 4);
    assert(reaction.liked == true && reaction.count == 1);
    reaction.complete(true, {}, 6);
    assert(reaction.count == 1);
    reaction.read(true, 7, 6);
    assert(reaction.count == 7);
    assert(reaction.begin(7));
    reaction.complete(false, 3, 8);
    assert(reaction.liked == false && reaction.count == 3);
    assert(reaction.begin(9));
    reaction.fail(10);
    assert(reaction.uncertain && ! reaction.pending && ! reaction.begin(11));
    reaction.read(true, 8, 9);
    assert(reaction.uncertain && reaction.liked == false);
    reaction.read(false, {}, 12);
    assert(! reaction.uncertain && ! reaction.count && reaction.begin(13));
    reaction.complete(true, {}, 14);
    assert(! reaction.count && reaction.liked == true);
    reaction.read(true, 0, 15);
    assert(reaction.begin(16));
    reaction.complete(false, {}, 17);
    assert(reaction.count == 0);
}
