#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

struct skiplist_t;
typedef struct skiplist_t skiplist_t;

struct skiplist_node_t;
typedef struct skiplist_node_t skiplist_node_t;

struct skiplist_node_t {
    skiplist_node_t * next;
    skiplist_node_t * prev;
    const uint8_t level; // the level of this node (the index in the array)
    const uint8_t num_levels; // the number of levels in this node array (the length of the array)
    int value; // data
};

skiplist_node_t * skiplist_head_of(skiplist_node_t * node) {
    skiplist_node_t * head = node - node->level;
    return head;
}

struct skiplist_t {
    skiplist_node_t * head;
    const uint8_t max_num_levels;
    const float p;
};

skiplist_t * new_skiplist(const uint8_t num_levels, const float p) {
    skiplist_t * self = malloc(sizeof(skiplist_t));
    const skiplist_t init = {.head = NULL, .max_num_levels = num_levels, .p = p};
    memcpy(self, &init, sizeof(skiplist_t));
    return self;
}

void free_skiplist(skiplist_t * self) {
    skiplist_node_t * head = self->head;
    while (head) {
        skiplist_node_t * next = head->next;
        free(head);
        head = next;
    }
    free(self);
}

size_t generate_level_for_new_node(const size_t max_num_levels, const float p) {
    static bool first = true;
    if (first) {
        first = false;
        return max_num_levels;
    }

    size_t level = 1;
    while ((float)(rand() % 100) / 100.f < p && level < max_num_levels) {
        ++level;
    }
    return level;
}

typedef struct {
    int level;
    int value;
    bool exists;
} skiplist_element_info_t;

static skiplist_node_t * _skiplist_find_headnode_predecessor(skiplist_node_t * node, const int value) {
    assert(node != NULL);
    assert(node->value <= value);

    while (node->level > 0) {

        while ((node->next == NULL || node->next->value > value) && node->level > 0) {
            --node;
        }

        while (node->next && node->next->value <= value) {
            node = node->next;
        }
    }
    return skiplist_head_of(node);
}

static const skiplist_node_t * _skiplist_find(const skiplist_t * const self, const int value) {
    if (self->head != NULL) {
        skiplist_node_t * head_of_pred = _skiplist_find_headnode_predecessor(self->head + self->head->num_levels - 1, value);
        if (head_of_pred->value == value) {
            return head_of_pred;
        }
    }
    return NULL;
}

skiplist_element_info_t skiplist_get_element_info(const skiplist_t * const self, const int value) {
    const skiplist_node_t * const head_of_node = _skiplist_find(self, value);
    if (head_of_node != NULL) {
        return (skiplist_element_info_t){ .level = head_of_node->level, .value = head_of_node->value, .exists = true };
    }
    return (skiplist_element_info_t){ .level = 0, .value = 0, .exists = false };
}

bool skiplist_contains(skiplist_t * self, const int value) {
    return skiplist_get_element_info(self, value).exists;
}

void skiplist_insert(skiplist_t * self, int value) {
    const uint8_t level_of_new_node = generate_level_for_new_node(self->max_num_levels, self->p);
    skiplist_node_t * new_node = malloc(sizeof(skiplist_node_t) * level_of_new_node);
    for (uint8_t level = 0; level < level_of_new_node; ++level) {
        skiplist_node_t init = {.next = NULL, .prev = NULL, .level = level, .num_levels = level_of_new_node, .value = value};
        memcpy(new_node + level, &init, sizeof(skiplist_node_t));
    }

    if (self->head == NULL) {
        self->head = new_node;
        return;
    }

    if (self->head->value > value) {
        // the new value should be the head
        const int tmp = value;
        value = self->head->value;
        for (uint8_t i = 0; i < self->head->num_levels; ++i) {
            self->head[i].value = tmp;
        }
        for (uint8_t i = 0; i < new_node->num_levels; ++i) {
            new_node[i].value = value;
        }
    }

    skiplist_node_t * prev_node = _skiplist_find_headnode_predecessor(self->head + self->head->num_levels - 1, value);
    for (uint8_t i = 0; i < level_of_new_node; ++i) {
        if (i == prev_node->num_levels) {
            while (prev_node->num_levels <= i) {
                prev_node = prev_node->prev;
            }
        }
        new_node[i].next = prev_node[i].next;
        new_node[i].prev = &prev_node[i];

        prev_node[i].next = &new_node[i];
        if (new_node[i].next != NULL) {
            new_node[i].next->prev = &new_node[i];
        }

    }

}

void skiplist_print(const skiplist_t * const self) {
    for (uint8_t level = 0; level < self->max_num_levels; ++level) {
        const skiplist_node_t * head_of_level = &self->head[level];
        printf("Level %d: ", level);
        while (head_of_level) {
            printf(" %d -> ", head_of_level->value);
            if (head_of_level->next) {
                assert(head_of_level->value <= head_of_level->next->value);
            }
            head_of_level = head_of_level->next;
        }
        printf(" NULL\n");
    }
}

void skiplist_print_top_only(const skiplist_t * const self) {
    const uint8_t top_level = self->head->num_levels - 1;

    const skiplist_node_t * head_of_level = &self->head[top_level];
    printf("Level %d: ", top_level);
    while (head_of_level) {
        printf(" %d -> ", head_of_level->value);
        if (head_of_level->next) {
            assert(head_of_level->value <= head_of_level->next->value);
        }
        head_of_level = head_of_level->next;
    }
    printf("NULL\n");
}

int main(void) {
    srand(getpid());
    printf("seed value: %d\n", getpid());

    skiplist_t * list = new_skiplist(4, 0.5);
    for (size_t i = 0; i < 1500; ++i) {
        skiplist_insert(list, rand() % 1000);
    }

    const bool contains = skiplist_contains(list, 222);
    printf("\nContains 222? %s\n", contains ? "yes" : "no");

    skiplist_print(list);
    free_skiplist(list);
    return 0;
}
