/*
四近傍ラベリングアルゴリズム
Ito Hajime
2020/07/19
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TARGET_PIXEL 1
#define NONTARGET_PIXEL 0
#define OUTSPACE NONTARGET_PIXEL
#define THRESHOLD_BRIGHTNESS 128
#define MAX_NUM 50000 /* この大きさで処理速度が大幅に変わる。適切な大きさにする必要あり。 */
#define EXCEPTION MAX_NUM + 1
#define LABEL_MAX_SIZE MAX_NUM / 2

typedef struct picture
{
    unsigned int **pixel_map;
    unsigned int row_size;
    unsigned int col_size;
    unsigned int target_pixel_sum;
} Picture;

void print_picture(Picture p);
void init_picture(Picture *p, unsigned int h, unsigned int w);
void init_lookup_table(unsigned int t[MAX_NUM][1]);
void push_lookup_table(unsigned int t[MAX_NUM][1], unsigned int index, unsigned int value);
void update_lookup_table(unsigned int t[MAX_NUM][1], unsigned int top_label_val, unsigned int left_label_val);
unsigned int get_lookup_table(unsigned int t[MAX_NUM][1], unsigned int index);
void resolve_conflict(Picture *p, unsigned int t[MAX_NUM][1], unsigned int compression_label[LABEL_MAX_SIZE]);
void compress_label(Picture *p, unsigned int compression_label[LABEL_MAX_SIZE], unsigned int lookup_table[MAX_NUM][1]);
void labeling(Picture *p, unsigned int t[MAX_NUM][1], unsigned int compression_label[LABEL_MAX_SIZE]);
void test_case(Picture p);
void FREE(Picture *p);

int main(void)
{
    char buf[MAX_NUM] = {0};
    unsigned int lookup_table[MAX_NUM][1] = {{0}};
    unsigned int height = 0;
    unsigned int width = 0;
    unsigned int compression_label[LABEL_MAX_SIZE] = {NONTARGET_PIXEL};
    unsigned int testcase_count = 0;

    Picture picture = {0, 0, 0};

    fgets(buf, sizeof(buf), stdin);
    sscanf(buf, "%u %u\n", &height, &width);

    for (testcase_count = 0; testcase_count < 1; testcase_count++)
    {
        srand((unsigned int)time(NULL));
        printf("time: %u\n", (unsigned int)time(NULL));
        init_picture(&picture, height, width);
        init_lookup_table(lookup_table);
        labeling(&picture, lookup_table, compression_label);
        compress_label(&picture, compression_label, lookup_table);
        test_case(picture);
        //print_picture(picture);
        FREE(&picture);
    }
    return 0;
}

/* pictureをコンソールに表示する */
void print_picture(Picture p)
{
    unsigned int r = 0;
    unsigned int c = 0;

    for (r = 0; r < p.row_size; r++)
    {
        printf("\n");
        for (c = 0; c < p.col_size; c++)
        {
            if ((OUTSPACE == p.pixel_map[r][c] || NONTARGET_PIXEL == p.pixel_map[r][c]))
                printf("  ..  ");
            else
                printf(" %4u ", p.pixel_map[r][c]);
        }
    }
    printf("\n\n");
}

/* pictureの初期化関数 */
void init_picture(Picture *p, unsigned int h, unsigned int w)
{
    unsigned int r = 0;
    unsigned int c = 0;
    unsigned char brightness = 0;
    /* 上下左右に余白を持たせておく */
    p->row_size = h + 2;
    p->col_size = w + 2;
    /* target_pixel_sumの初期化 */
    p->target_pixel_sum = 0;

    /* メモリ割り当て */
    p->pixel_map = (unsigned int **)malloc(p->row_size * sizeof(unsigned int *));
    for (r = 0; r < p->row_size; r++)
        p->pixel_map[r] = (unsigned int *)malloc(p->col_size * sizeof(unsigned int));

    /* 全ての画素にOUTSPACEを割り当てる */
    for (r = 0; r < p->row_size; r++)
        for (c = 0; c < p->col_size; c++)
            p->pixel_map[r][c] = OUTSPACE;

    /* ランダムに画素値を決定し、閾値を基に二値化する */
    for (r = 1; r < p->row_size - 1; r++)
        for (c = 1; c < p->col_size - 1; c++)
        {
            brightness = (rand() % 256);
            if (brightness <= THRESHOLD_BRIGHTNESS)
            {
                p->pixel_map[r][c] = TARGET_PIXEL;
                p->target_pixel_sum += 1;
            }
            else
            {
                p->pixel_map[r][c] = NONTARGET_PIXEL;
            }
        }
}

/* loolup_tableの初期化関数 */
void init_lookup_table(unsigned int t[MAX_NUM][1])
{
    /* lookup_tableの初期化 */
    unsigned int l = 0;
    for (l = 0; l < MAX_NUM; l++)
    {
        t[l][0] = EXCEPTION;
    }
    t[0][0] = MAX_NUM;
}

/* lookup_tableに値を挿入する関数 */
void push_lookup_table(unsigned int t[MAX_NUM][1], unsigned int index, unsigned int value)
{
    /* 例外処理 */
    if (MAX_NUM < index)
    {
        fprintf(stderr, "エラー：indexの値が%u、valueの値が%uです。データの挿入に失敗しました。\n", index, value);
        return;
    }

    if (MAX_NUM == index)
        return;

    /* lookup_tableに値を挿入する(0番目は使わないのでスキップする) */
    if (0 == index)
        t[0][0] = MAX_NUM;
    else
        t[index][0] = value;
}

/* 着目画素の上・左にある画素のラベル値が、着目画素のラベル値よりも大きい場合にlookup_tableを更新する */
void update_lookup_table(unsigned int t[MAX_NUM][1], unsigned int top_label_val, unsigned int left_label_val)
{
    unsigned int max_label_val = 0; /* 着目画素のラベル値と比べて一番大きい値 */
    unsigned int min_label_val = 0; /* 着目画素のラベル値と比べて一番小さい値 */
    unsigned int pat = 0;
    unsigned int update_label_val = 0;

    /* NONTARGET_PIXELやOUTSPACEを無視して最大値と最小値を求める */
    /* 両方に適切なデータが存在あると想定した場合 */
    if (top_label_val > left_label_val)
    {
        max_label_val = top_label_val;
        min_label_val = left_label_val;
    }

    if (left_label_val > top_label_val)
    {
        max_label_val = left_label_val;
        min_label_val = top_label_val;
    }

    /* 片方のみに適切なデータが存在あると想定した場合(優先) */
    if (MAX_NUM == top_label_val)
    {
        /* top_pixel_valがMAX_NUMなら、leftはMAX_NUM以外のデータが入っている */
        max_label_val = left_label_val;
        min_label_val = left_label_val;
    }

    if (MAX_NUM == left_label_val) //(NONTARGET_PIXEL == left_label_val || OUTSPACE == left_label_val)
    {
        /* left_pixel_valがMAX_NUMなら、topはMAX_NUM以外のデータが入っている */
        max_label_val = top_label_val;
        min_label_val = top_label_val;
    }

    /* 大きいラベル値のlookup_tableの値を辿って、適切な位置のlookup_tableの値を更新する */
    /* 例) 適切な値の更新 */
    /* lookup_table */
    /* 1 -> 1 */
    /* 2 -> 2 */
    /* 3 -> 2 */
    /* 4 -> 3 */
    /* max_pixel_val 4  */
    /* min_pixel_val 1 */
    /* 以上が得られたとき、lookup_tableは2->1と更新する */
    /* ここで、2がupdate_label_valである。 */
    update_label_val = max_label_val;

    for (pat = MAX_NUM; pat > 0; pat--)
    {
        if (get_lookup_table(t, update_label_val) < update_label_val)
            update_label_val = get_lookup_table(t, update_label_val); /* 辿るべき、lookup_tableの手掛かりでupdate_label_valを更新する */

        if (get_lookup_table(t, update_label_val) == update_label_val)
        {
            if (get_lookup_table(t, update_label_val) > min_label_val)
                push_lookup_table(t, update_label_val, min_label_val); /* 更新 */
            break;                                                     /* 終了 */
        }
    }
}

/* lookup_tableの値を取得する関数 */
unsigned int get_lookup_table(unsigned int t[MAX_NUM][1], unsigned int index)
{
    /* 例外処理 */
    if (MAX_NUM < index)
    {
        fprintf(stderr, "エラー：indexの値が%uです。データの取得に失敗しました。\n", index);
        return 0;
    }

    if (MAX_NUM == index)
        return 0;

    /* lookup_tableに値を返す(0番目は使わないのでスキップする) */
    return t[index][0];
}

/* 衝突処理(繋がっているけどラベル値が違うピクセルに適切な値を割り当てる)を行う関数 */
void resolve_conflict(Picture *p, unsigned int t[MAX_NUM][1], unsigned int compression_label[LABEL_MAX_SIZE])
{
    unsigned int r = 1;
    unsigned int c = 1;
    unsigned int pat = 0;
    unsigned int update_label_val = 0;

    /* 衝突処理用ラスタスキャン */
    for (r = 1; r < p->row_size - 1; r++)
        for (c = 1; c < p->col_size - 1; c++)
        {
            if ((NONTARGET_PIXEL == p->pixel_map[r][c] || OUTSPACE == p->pixel_map[r][c]))
                continue;

            update_label_val = p->pixel_map[r][c];
            /* ここら辺はラベル割り当てとほとんど同じ */
            for (pat = MAX_NUM; pat > 0; pat--)
            {
                if (get_lookup_table(t, update_label_val) < update_label_val)
                {
                    update_label_val = get_lookup_table(t, update_label_val);
                }

                if (get_lookup_table(t, update_label_val) == update_label_val)
                {
                    p->pixel_map[r][c] = get_lookup_table(t, update_label_val);
                    break;
                }
            }
        }
}

/* ラベル値を圧縮して再度ラベリングする(必須ではない) */
void compress_label(Picture *p, unsigned int compression_label[LABEL_MAX_SIZE], unsigned int lookup_table[MAX_NUM][1])
{
    unsigned int r = 1;
    unsigned int c = 1;
    unsigned int pat = 0;

    for (r = 1; r < MAX_NUM; r++)
    {
        if (EXCEPTION == get_lookup_table(lookup_table, r))
            break;

        if (get_lookup_table(lookup_table, r) == r)
        {
            compression_label[c] = r;
            c += 1;
        }
    }

    for (r = 1; r < p->row_size - 1; r++)
        for (c = 1; c < p->col_size - 1; c++)
        {
            if ((NONTARGET_PIXEL == p->pixel_map[r][c] || OUTSPACE == p->pixel_map[r][c]))
                continue;
            for (pat = 1; pat < LABEL_MAX_SIZE; pat++)
            {
                if (NONTARGET_PIXEL == compression_label[pat])
                    continue;

                if (p->pixel_map[r][c] == compression_label[pat])
                    p->pixel_map[r][c] = pat;
            }
        }
}

/* ラベルの割り当てと、lookup_tableの作成を行う関数 */
void labeling(Picture *p, unsigned int t[MAX_NUM][1], unsigned int compression_label[LABEL_MAX_SIZE])
{
    unsigned int t_pat = 1; /* 0はスキップ */
    unsigned int top_pixel_val = 0;
    unsigned int left_pixel_val = 0;
    unsigned int top_label_val = 0;
    unsigned int left_label_val = 0;
    unsigned int r = 1;
    unsigned int c = 1;

    /* ラスタスキャン */
    for (r = 1; r < p->row_size - 1; r++)
    {
        for (c = 1; c < p->col_size - 1; c++)
        {
            /* ターゲット画素でないならスキップ */
            if (NONTARGET_PIXEL == p->pixel_map[r][c])
                continue;

            top_pixel_val = p->pixel_map[r][c - 1];
            left_pixel_val = p->pixel_map[r - 1][c];

            if ((NONTARGET_PIXEL == top_pixel_val && NONTARGET_PIXEL == left_pixel_val) || (OUTSPACE == top_pixel_val && OUTSPACE == left_pixel_val))
            {
                /* 上の画素値と左の画素値が両方ターゲット画素ではない、もしくは両方余白な場合 */
                push_lookup_table(t, t_pat, t_pat);              /* lookup_tableに新しい値を割り振る */
                p->pixel_map[r][c] = get_lookup_table(t, t_pat); /* 着目画素値のラベル値を更新 */
                t_pat += 1;
            }
            else
            {
                /* どちらかにターゲット画素値が存在する場合 */
                /* 上・左の画素には既にラベルが割り当てられているので、それぞれのlookup_tableを参照する */
                top_label_val = get_lookup_table(t, top_pixel_val);
                left_label_val = get_lookup_table(t, left_pixel_val);

                /* 上・左のラベル値のうち小さいほうを着目画素のラベル値にする */
                p->pixel_map[r][c] = (top_label_val > left_label_val) ? left_label_val : top_label_val;

                /* lookup_tableの値を更新する */
                update_lookup_table(t, top_label_val, left_label_val);
            }
        }
    }

    /* 逆ラスタスキャン */
    for (r = p->row_size - 1; r > 0; r--)
    {
        for (c = p->col_size - 1; c > 0; c--)
        {
            /* ターゲット画素でないならスキップ */
            if (NONTARGET_PIXEL == p->pixel_map[r][c])
                continue;

            top_pixel_val = p->pixel_map[r][c + 1];
            left_pixel_val = p->pixel_map[r + 1][c];

            if ((NONTARGET_PIXEL == top_pixel_val && NONTARGET_PIXEL == left_pixel_val) || (OUTSPACE == top_pixel_val && OUTSPACE == left_pixel_val))
                continue;

            /* どちらかにターゲット画素値が存在する場合 */
            /* 上・左の画素には既にラベルが割り当てられているので、それぞれのlookup_tableを参照する */
            top_label_val = get_lookup_table(t, top_pixel_val);
            left_label_val = get_lookup_table(t, left_pixel_val);

            /* 上・左のラベル値のうち小さいほうを着目画素のラベル値にする */
            p->pixel_map[r][c] = (top_label_val > left_label_val) ? left_label_val : top_label_val;

            /* lookup_tableの値を更新する */
            update_lookup_table(t, top_label_val, left_label_val);
        }
    }

    resolve_conflict(p, t, compression_label);
}

/* テストケース用の関数 */
void test_case(Picture p)
{
    char flag = 0;
    unsigned int search_table[4];
    unsigned int r = 1;
    unsigned int c = 1;
    unsigned int pat = 0;
    /* debug */
    unsigned debug = 0;
    unsigned debug_target_pixel_sum = 0;

    /* 出力結果が間違っていないかテストするための処理 */
    /* ラスタスキャン */
    for (r = 1; r < p.row_size - 1; r++)
        for (c = 1; c < p.col_size - 1; c++)
        {
            if ((NONTARGET_PIXEL == p.pixel_map[r][c] || OUTSPACE == p.pixel_map[r][c]))
                continue;

            debug_target_pixel_sum += 1;

            search_table[0] = p.pixel_map[r - 1][c];
            search_table[1] = p.pixel_map[r][c - 1];
            search_table[2] = p.pixel_map[r + 1][c];
            search_table[3] = p.pixel_map[r][c + 1];

            for (pat = 0; pat < 4; pat++)
            {
                if ((search_table[pat] != p.pixel_map[r][c] && search_table[pat] > 0))
                {
                    flag = 1;
                    debug = search_table[pat];
                }
            }
        }

    if ((0 == flag) && (debug_target_pixel_sum == p.target_pixel_sum))
        printf("success\n");
    else
    {
        printf("error -> %u\n", debug);
    }
}

void FREE(Picture *p)
{
    unsigned int r = 0;
    for (r = 0; r < p->row_size; r++)
        free(p->pixel_map[r]);
    free(p->pixel_map);
}