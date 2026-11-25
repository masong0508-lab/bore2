// Small shared rules for optional neighbor activities. Rest is 0..100.
#pragma once
enum { SA_SHARE, SA_HELP, SA_STORY };
enum { SA_OK, SA_NO_MONEY, SA_TIRED };
#define SA_SNACK_PRICE 5
#define SA_SNACK_FOOD 15
#define SA_HELP_REST 12
#define SA_STORY_FUN 12
static inline int social_activity_check(int activity, long money, int rest) {
    if (activity == SA_SHARE && money < SA_SNACK_PRICE) return SA_NO_MONEY;
    if (activity == SA_HELP && rest < SA_HELP_REST + 5) return SA_TIRED;
    return SA_OK;
}
