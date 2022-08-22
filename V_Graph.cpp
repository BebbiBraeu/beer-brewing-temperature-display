#include <algorithm>
#include "V_Graph.h"

Dataset::Dataset() : x{}, y{}
{

}

void Dataset::add_data(float x_val, float y_val)
{
    x.push_back(x_val);
    y.push_back(y_val);
}   

float Dataset::get_max_x()
{
    const auto max = std::max_element(std::begin(x), std::end(x));
    return *max;
}

float Dataset::get_min_x()
{
    const auto min = std::min_element(std::begin(x), std::end(x));
    return *min;
}

float Dataset::get_max_y()
{
    const auto max = std::max_element(std::begin(y), std::end(y));
    return *max;
}

float Dataset::get_min_y()
{
    const auto min = std::min_element(std::begin(y), std::end(y));
    return *min;
}

int Dataset::get_size()
{
    return x.size();
}

int32_t Dataset::get_x_val(int id)
{
    return x[id];
}

int32_t Dataset::get_y_val(int id)
{
    return y[id];
}



DatasetCollection::DatasetCollection() : vec_datasets{}
{

}

int DatasetCollection::create_dataset()
{
    vec_datasets.push_back(new Dataset);
    return count_datasets() - 1;
}

int DatasetCollection::count_datasets()
{
    return vec_datasets.size();
}

void DatasetCollection::add_data_to_dataset(int id, float x, float y)
{
    vec_datasets[id]->add_data(x, y);
}

float DatasetCollection::get_min_x_all()
{
    float temp_min = vec_datasets[0]->get_min_x();

    for(int id = 0; id < count_datasets(); id++)
    {
        temp_min = (vec_datasets[id]->get_min_x()<temp_min) ? vec_datasets[id]->get_min_x():temp_min; 
    }

    return temp_min;
}

float DatasetCollection::get_max_x_all()
{
    float temp_max = vec_datasets[0]->get_max_x();

    for(int id = 0; id < count_datasets(); id++)
    {
        temp_max = (vec_datasets[id]->get_max_x()>temp_max) ? vec_datasets[id]->get_max_x():temp_max; 
    }

    return temp_max;
}

float DatasetCollection::get_min_y_all()
{
    float temp_min = vec_datasets[0]->get_min_y();

    for(int id = 0; id < count_datasets(); id++)
    {
        temp_min = (vec_datasets[id]->get_min_y()<temp_min) ? vec_datasets[id]->get_min_y():temp_min; 
    }

    return temp_min;
}

float DatasetCollection::get_max_y_all()
{
    float temp_max = vec_datasets[0]->get_max_y();

    for(int id = 0; id < count_datasets(); id++)
    {
        temp_max = (vec_datasets[id]->get_max_y()>temp_max) ? vec_datasets[id]->get_max_y():temp_max; 
    }

    return temp_max;
}

Dataset* DatasetCollection::get_ds_p(int id)
{
    return vec_datasets[id];
}


V_Graph::V_Graph(V_Display* TFT, int32_t x, int32_t y, int32_t w, int32_t h, bool swap_y) :
    dspl {TFT}, width {w}, height {h}, swap{swap_y}, crn_x {x}, crn_y {y}, margin_x{30}, margin_y{20}, ticks_x{5}, ticks_y{5}, limits_x_min{0.0}, limits_x_max{1.0}, limits_y_min{0.0}, limits_y_max{1.0}
{
    org_x = x + margin_x;
    org_y = y + margin_y;
    len_x = width - margin_x;
    len_y = height - margin_y;
}

void V_Graph::Axis()
{
    //dspl->drawFastHLine(crn_x, crn_y, width, TFT_GREEN);
    //dspl->drawFastVLine(crn_x, crn_y, height, TFT_GREEN);
    //dspl->drawFastHLine(crn_x, crn_y+height, width, TFT_GREEN);
    //dspl->drawFastVLine(crn_x+width, crn_y, height, TFT_GREEN);

    dspl->setTextColor(TFT_WHITE);
    
    dspl->drawLine(org_x, flip_y(org_y), org_x + (width-margin_x), flip_y(org_y), TFT_WHITE);
    dspl->drawLine(org_x, flip_y(org_y), org_x, flip_y(org_y + (height-margin_y)), TFT_WHITE);

    for(int i = ticks_x-1; i >= 0; i--)
    {
        dspl->drawLine(org_x+len_x-i*(len_x/ticks_x), flip_y(org_y-margin_y/2), org_x+len_x-i*(len_x/ticks_x), flip_y(org_y-margin_y/2 + (margin_y/2)), TFT_WHITE);
        String text = String((1 - i / static_cast<float>(ticks_x)) * (limits_x_max - limits_x_min) + limits_x_min, 0);
        if(swap)
        {
            dspl->printText(org_x+len_x-i*(len_x/ticks_x)-50, flip_y(crn_y)-8, text, 1, 3, 50);
        }
        else
        {
            dspl->printText(org_x+len_x-i*(len_x/ticks_x)-50, crn_y, text, 1, 3, 50);
        }
    }
    for(int i = ticks_y-1; i >= 0; i--)
    {
        dspl->drawLine(org_x-margin_x/2, flip_y(org_y+len_y-i*(len_y/ticks_y)), org_x-margin_x/2 + (margin_x/2), flip_y(org_y+len_y-i*(len_y/ticks_y)), TFT_WHITE);
        String text = String((1 - i / static_cast<float>(ticks_y)) * (limits_y_max - limits_y_min) + limits_y_min, 1);
        if(swap)
        {
            dspl->printText(org_x - 50, flip_y(org_y+len_y-i*(len_y/ticks_y))+2, text, 1, 3, 50);
        }
        else
        {
            dspl->printText(org_x - 50, org_y+len_y-i*(len_y/ticks_y)-8, text, 1, 3, 50);
        }
    }
}

void V_Graph::set_ticks(int tcks_x, int tcks_y)
{
    ticks_x = tcks_x;
    ticks_y = tcks_y;
}

void V_Graph::set_limits(float lim_x_min, float lim_x_max, float lim_y_min, float lim_y_max)
{
    limits_x_min = lim_x_min;
    limits_x_max = lim_x_max;
    limits_y_min = lim_y_min;
    limits_y_max = lim_y_max;
}

void V_Graph::set_limits_auto(DatasetCollection* dsc, float factor_x, float factor_y)
{
    float dif_x = dsc->get_max_x_all() - dsc->get_min_x_all();
    float dif_y = dsc->get_max_y_all() - dsc->get_min_y_all();
    set_limits(dsc->get_min_x_all() - factor_x * dif_x, dsc->get_max_x_all() + factor_x * dif_x, dsc->get_min_y_all() - factor_y * dif_y, dsc->get_max_y_all() + factor_y * dif_y);
}

void V_Graph::draw_dataset(Dataset* ds, uint32_t color)
{
    int num = ds->get_size();
    for(int i = 0; i < num-1; i++)
    {
        dspl->drawLine(get_x_pos(ds->get_x_val(i)), get_y_pos(ds->get_y_val(i)), get_x_pos(ds->get_x_val(i+1)), get_y_pos(ds->get_y_val(i+1)), color);
    }
}

int32_t V_Graph::get_x_pos(float x)
{
    return static_cast<int32_t>((x -limits_x_min) / (limits_x_max-limits_x_min) * (width-margin_x) + org_x);
}

int32_t V_Graph::get_y_pos(float y)
{
    return flip_y(static_cast<int32_t>((y - limits_y_min) / (limits_y_max-limits_y_min) * (height-margin_y) + org_y));
}

float V_Graph::round_digits(float val)
{
    return static_cast<float>(static_cast<int>(val * 10.)) / 10.;
}

int32_t V_Graph::flip_y(int32_t old_y)
{
    if(swap)
    {
        return (crn_y + height) - (old_y - crn_y);
    }
    else
    {
        return old_y;
    }
    
}