#ifndef V_GRAPH_H
#define V_GRAPH_H
#include <Arduino.h>
#include "V_Display.h"
#include <vector>


class Dataset
{
    private:
        std::vector<float> x;
        std::vector<float> y;
    public:
        Dataset();
        void add_data(float x, float y);
        float get_max_x();
        float get_min_x();
        float get_max_y();
        float get_min_y();
        int get_size();
        int32_t get_x_val(int id);
        int32_t get_y_val(int id);
};

class DatasetCollection
{
    private:
        std::vector<Dataset*> vec_datasets;
    public:
        DatasetCollection();
        int create_dataset();
        int count_datasets();
        void add_data_to_dataset(int id, float x, float y);
        float get_max_x_all();
        float get_min_x_all();
        float get_max_y_all();
        float get_min_y_all();
        Dataset* get_ds_p(int id);
};

class V_Graph
{
    private:
        V_Display* dspl;
        int32_t width;
        int32_t height;        
        bool swap;
        int32_t crn_x;
        int32_t crn_y;
        int32_t org_x;
        int32_t org_y;
        int32_t margin_x;
        int32_t margin_y;
        int32_t len_x;
        int32_t len_y;
        int ticks_x;
        int ticks_y;
        float limits_x_min;
        float limits_x_max;
        float limits_y_min;
        float limits_y_max;

        float round_digits(float val);
        int32_t get_x_pos(float x);
        int32_t get_y_pos(float y);
        int32_t flip_y(int32_t old_y);

    public:
        V_Graph(V_Display* TFT, int32_t x, int32_t y, int32_t w, int32_t h, bool swap_y = true);
        void set_ticks(int ticks_x, int ticks_y);
        void set_limits(float lim_x_min, float lim_x_max, float lim_y_min, float lim_y_max);
        void set_limits_auto(DatasetCollection* dsc, float factor_x = 0.1, float factor_y = 0.1);
        void Axis();
        void draw_dataset(Dataset* ds, uint32_t color);
};

#endif