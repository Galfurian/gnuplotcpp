/// @file gnuplot.hpp
/// @brief A C++ interface to gnuplot.
/// @details
/// The interface uses pipes and so won't run on a system that doesn't have
/// POSIX pipe support Tested on Windows (MinGW and Visual C++) and Linux (GCC)

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for std::ostringstream
#include <stdexcept>
#include <cstdio>
#include <cstdlib> // for getenv()
#include <list>    // for std::list

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
//defined for 32 and 64-bit environments
#include <io.h> // for _access(), _mktemp()

#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
//all UNIX-like OSs (Linux, *BSD, MacOSX, Solaris, ...)
#include <unistd.h> // for access(), mkstemp()

#else
#error unsupported or unknown operating system
#endif

namespace gnuplotcpp
{

/// Enum representing the various plotting styles available in Gnuplot.
enum class plot_style_t {
    none,          ///< Default fallback style (points).
    lines,         ///< Lines connecting the data points.
    points,        ///< Individual data points.
    lines_points,  ///< Lines connecting data points with points marked.
    impulses,      ///< Vertical lines from the x-axis to the data points.
    dots,          ///< Small dots for data points.
    steps,         ///< Stepwise connection of data points.
    fsteps,        ///< Finite steps between data points.
    histeps,       ///< Histogram-like steps between data points.
    boxes,         ///< Boxes for histogram-like data.
    filled_curves, ///< Filled areas under curves.
    histograms,    ///< Histograms.
};

/// Enum representing the smoothing styles available in Gnuplot.
enum class smooth_style_t {
    none,      ///< No smoothing (default).
    unique,    ///< Unique smoothing.
    frequency, ///< Frequency-based smoothing.
    csplines,  ///< Cubic spline interpolation.
    acsplines, ///< Approximation cubic splines.
    bezier,    ///< Bezier curve smoothing.
    sbezier,   ///< Subdivided Bezier smoothing.
};

/// Contour type options for Gnuplot
enum class contour_type_t {
    none,    ///< Disables contouring
    base,    ///< Contours on the base (XY-plane)
    surface, ///< Contours on the surface
    both,    ///< Contours on both base and surface
};

/// Contour parameter options for Gnuplot
enum class contour_param_t {
    levels,    // Number of contour levels
    increment, // Contour increment settings
    discrete,  // Specific discrete contour levels
};

/// @brief Custom exception for Gnuplot-related errors.
/// Provides detailed error messages and additional context if necessary.
class GnuplotException : public std::runtime_error {
public:
    /// @brief Constructor accepting an error message.
    /// @param msg The error message describing the exception.
    explicit GnuplotException(const std::string &msg) : std::runtime_error("[GnuplotException] " + msg)
    {
    }

    /// @brief Constructor accepting an error message and context information.
    /// @param msg The error message describing the exception.
    /// @param context Additional context for the error.
    GnuplotException(const std::string &msg, const std::string &context)
        : std::runtime_error("[GnuplotException] " + msg + " | Context: " + context)
    {
    }

    /// @brief Retrieve the error message without the GnuplotException prefix.
    /// @return The plain error message.
    std::string getPlainMessage() const noexcept
    {
        return std::string(what()).substr(18); // Skips the "[GnuplotException] " prefix.
    }
};

/// @brief Main Gnuplot class for managing plots.
class Gnuplot {
public:
    /// @brief Constructs a Gnuplot session with a specified plot style.
    /// @param style The plotting style (default is "points").
    Gnuplot(plot_style_t style = plot_style_t::none);

    /// @brief Constructs a Gnuplot session and plots a single vector.
    /// @param x The data points to plot.
    /// @param title The title of the plot (default is empty).
    /// @param style The plotting style (default is "points").
    /// @param labelx The label for the x-axis (default is "x").
    /// @param labely The label for the y-axis (default is "y").
    Gnuplot(const std::vector<double> &x,
            const std::string &title  = "",
            plot_style_t style        = plot_style_t::none,
            const std::string &labelx = "x",
            const std::string &labely = "y");

    /// @brief Constructs a Gnuplot session and plots paired x and y vectors.
    /// @param x The data points for the x-axis.
    /// @param y The data points for the y-axis.
    /// @param title The title of the plot (default is empty).
    /// @param style The plotting style (default is "points").
    /// @param labelx The label for the x-axis (default is "x").
    /// @param labely The label for the y-axis (default is "y").
    Gnuplot(const std::vector<double> &x,
            const std::vector<double> &y,
            const std::string &title  = "",
            plot_style_t style        = plot_style_t::none,
            const std::string &labelx = "x",
            const std::string &labely = "y");

    /// @brief Constructs a Gnuplot session and plots triples (x, y, z) vectors.
    /// @param x The data points for the x-axis.
    /// @param y The data points for the y-axis.
    /// @param z The data points for the z-axis.
    /// @param title The title of the plot (default is empty).
    /// @param style The plotting style (default is "points").
    /// @param labelx The label for the x-axis (default is "x").
    /// @param labely The label for the y-axis (default is "y").
    /// @param labelz The label for the z-axis (default is "z").
    Gnuplot(const std::vector<double> &x,
            const std::vector<double> &y,
            const std::vector<double> &z,
            const std::string &title  = "",
            plot_style_t style        = plot_style_t::none,
            const std::string &labelx = "x",
            const std::string &labely = "y",
            const std::string &labelz = "z");

    /// @brief Destructor to clean up and delete temporary files.
    ~Gnuplot();

    /// @brief Sets the Gnuplot path manually.
    /// @details For Windows, ensure the path uses forward slashes ('/') instead of backslashes ('\').
    /// @param path The Gnuplot executable path.
    /// @return `true` if the path was successfully set, `false` otherwise.
    static bool set_gnuplot_path(const std::string &path);

    /// @brief Sets the default terminal type for displaying plots.
    /// @details Defaults are:
    /// - Windows: "win"
    /// - Linux: "x11"
    /// - macOS: "aqua"
    /// @param type The terminal type to set.
    /// @return void
    static void set_terminal_std(const std::string &type);

    /// @brief Sends a command to the Gnuplot session.
    /// @param cmdstr The command string to send to Gnuplot.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &send_cmd(const std::string &cmdstr);

    /// @brief Sends a command to an active Gnuplot session using the `<<` operator.
    /// @param cmdstr The command string to send to Gnuplot.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &operator<<(const std::string &cmdstr)
    {
        this->send_cmd(cmdstr);
        return (*this);
    }

    /// @brief Displays the plot on the screen using the default terminal type.
    /// @details The terminal type defaults to:
    /// - Windows: "win"
    /// - Linux: "x11"
    /// - macOS: "aqua"
    /// @return A reference to the current Gnuplot object.
    Gnuplot &showonscreen();

    /// @brief Saves the current plot to a file.
    /// @param filename The name of the output file.
    /// @param terminal The terminal type for saving (default is "ps").
    /// @return A reference to the current Gnuplot object.
    Gnuplot &savetofigure(const std::string filename, const std::string terminal = "ps");

    /// Sets the plotting style for the current Gnuplot session.
    /// @param style The plot_style_t enum value representing the desired plotting style.
    /// @return Reference to the current Gnuplot object.
    Gnuplot &set_style(plot_style_t style);

    /// @brief Sets the smoothing style for the current Gnuplot session.
    /// @param style The smooth_style enum value representing the desired smoothing style.
    /// @return Reference to the current Gnuplot object.
    Gnuplot &set_smooth(smooth_style_t style = smooth_style_t::csplines);

    /// @brief Sets the size of points used in plots.
    /// @param pointsize The size of the points (default is 1.0).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_pointsize(const double pointsize = 1.0);

    /// @brief Sets the line width for the current Gnuplot session.
    /// @param width The desired line width. Must be greater than 0.
    /// @return Reference to the current Gnuplot object.
    Gnuplot &set_line_width(double width);

    /// @brief Enables the grid for plots.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_grid();

    /// @brief Disables the grid for plots.
    /// @details The grid is not enabled by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_grid();

    /// @brief Enables multiplot mode for displaying multiple plots in one session.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_multiplot();

    /// @brief Disables multiplot mode.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_multiplot();

    /// @brief Sets the sampling rate for plotting functions or interpolating data.
    /// @param samples The number of samples (default is 100).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_samples(const int samples = 100);

    /// @brief Sets the isoline density for plotting surfaces in 3D plots.
    /// @param isolines The number of isolines (default is 10).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_isosamples(const int isolines = 10);

    /// @brief Sets the contour type for Gnuplot.
    /// @param type The desired contour type (base, surface, both, none).
    /// @return Reference to the current Gnuplot object.
    Gnuplot &set_contour_type(contour_type_t type);

    /// @brief Configures contour levels based on the specified parameter type.
    /// @param param The desired contour parameter type (levels, increment, discrete).
    /// @return Reference to the current Gnuplot object.
    Gnuplot &set_contour_param(contour_param_t param);

    /// @brief Sets the number of contour levels.
    /// @param levels The number of contour levels.
    /// @return Reference to the current Gnuplot object.
    Gnuplot &set_contour_levels(int levels);

    /// @brief Sets the contour increment range and step size.
    /// @param start The start value of the increment range.
    /// @param step The step size for the increments.
    /// @param end The end value of the increment range.
    /// @return Reference to the current Gnuplot object.
    Gnuplot &set_contour_increment(double start, double step, double end);

    /// @brief Sets discrete contour levels.
    /// @param levels A vector of specific levels to be used as contours.
    /// @return Reference to the current Gnuplot object.
    Gnuplot &set_contour_discrete_levels(const std::vector<double> &levels);

    /// @brief Sends the configured contour commands to Gnuplot.
    /// @return Reference to the current Gnuplot object.
    Gnuplot &apply_contour_settings();

    /// @brief Enables hidden line removal for surface plotting in 3D plots.
    /// @details Hidden line removal improves the visualization of surfaces by hiding lines obscured by the surface.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_hidden3d();

    /// @brief Disables hidden line removal for surface plotting in 3D plots.
    /// @details Hidden line removal is not enabled by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_hidden3d();

    /// @brief Disables contour drawing for surfaces in 3D plots.
    /// @details Contour drawing is not enabled by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_contour();

    /// @brief Enables the display of surfaces in 3D plots.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_surface();

    /// @brief Disables the display of surfaces in 3D plots.
    /// @details Surface display is enabled by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_surface();

    /// @brief Enables the legend and sets its position in the plot.
    /// @details Available positions:
    /// - `inside` or `outside`
    /// - `left`, `center`, or `right`
    /// - `top`, `center`, or `bottom`
    /// - `nobox` or `box`
    /// @param position The position of the legend (default is "default").
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_legend(const std::string &position = "default");

    /// @brief Disables the legend in the plot.
    /// @details The legend is enabled by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_legend();

    /// @brief Sets the title of the plot.
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_title(const std::string &title = "");

    /// @brief Clears the title of the plot.
    /// @details The title is not set by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_title();

    /// @brief Sets the label for the x-axis.
    /// @param label The label for the x-axis (default is "x").
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_xlabel(const std::string &label = "x");

    /// @brief Sets the label for the y-axis.
    /// @param label The label for the y-axis (default is "y").
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_ylabel(const std::string &label = "y");

    /// @brief Sets the label for the z-axis.
    /// @param label The label for the z-axis (default is "z").
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_zlabel(const std::string &label = "z");

    /// @brief Sets the range for the x-axis.
    /// @param iFrom The starting value of the range.
    /// @param iTo The ending value of the range.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_xrange(const double iFrom, const double iTo);

    /// @brief Sets the range for the y-axis.
    /// @param iFrom The starting value of the range.
    /// @param iTo The ending value of the range.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_yrange(const double iFrom, const double iTo);

    /// @brief Sets the range for the z-axis.
    /// @param iFrom The starting value of the range.
    /// @param iTo The ending value of the range.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_zrange(const double iFrom, const double iTo);
    /// @brief Enables autoscaling for the x-axis.
    /// @details Autoscaling is enabled by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_xautoscale();

    /// @brief Enables autoscaling for the y-axis.
    /// @details Autoscaling is enabled by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_yautoscale();

    /// @brief Enables autoscaling for the z-axis.
    /// @details Autoscaling is enabled by default.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_zautoscale();

    /// @brief Enables logarithmic scaling for the x-axis.
    /// @details Logarithmic scaling is not enabled by default.
    /// @param base The base of the logarithm (default is 10).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_xlogscale(const double base = 10);

    /// @brief Enables logarithmic scaling for the y-axis.
    /// @details Logarithmic scaling is not enabled by default.
    /// @param base The base of the logarithm (default is 10).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_ylogscale(const double base = 10);

    /// @brief Enables logarithmic scaling for the z-axis.
    /// @details Logarithmic scaling is not enabled by default.
    /// @param base The base of the logarithm (default is 10).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_zlogscale(const double base = 10);

    /// @brief Disables logarithmic scaling for the x-axis.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_xlogscale();

    /// @brief Disables logarithmic scaling for the y-axis.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_ylogscale();

    /// @brief Disables logarithmic scaling for the z-axis.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &unset_zlogscale();

    /// @brief Sets the palette color range for plots.
    /// @details The palette range is used to map data values to colors in plots.
    /// Autoscaling is enabled by default, but this function allows manual control of the range.
    /// @param iFrom The starting value of the color range.
    /// @param iTo The ending value of the color range.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &set_cbrange(const double iFrom, const double iTo);

    /// @brief Plots data from a file as a single vector.
    /// @param filename The name of the file containing the data.
    /// @param column The column in the file to plot (default is 1).
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &plotfile_x(const std::string &filename, const unsigned int column = 1, const std::string &title = "");

    /// @brief Plots a single vector of data.
    /// @tparam X The type of the data in the vector.
    /// @param x The data to plot.
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    template <typename X>
    Gnuplot &plot_x(const X &x, const std::string &title = "");

    /// @brief Plots multiple vectors with separate titles.
    /// @tparam X The type of the data in the vectors.
    /// @param x The vectors of data to plot.
    /// @param title The titles for each vector.
    /// @return A reference to the current Gnuplot object.
    template <typename X>
    Gnuplot &plot_x(const std::vector<X> &x, const std::vector<std::string> &title);

    /// @brief Plots x, y pairs of data from a file.
    /// @param filename The name of the file containing the data.
    /// @param column_x The column for the x values (default is 1).
    /// @param column_y The column for the y values (default is 2).
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &plotfile_xy(const std::string &filename,
                         const unsigned int column_x = 1,
                         const unsigned int column_y = 2,
                         const std::string &title    = "");

    /// @brief Plots x, y pairs of data.
    /// @tparam X The type of the x data.
    /// @tparam Y The type of the y data.
    /// @param x The x values.
    /// @param y The y values.
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    template <typename X, typename Y>
    Gnuplot &plot_xy(const X &x, const Y &y, const std::string &title = "");

    /// @brief Plots x, y pairs with error bars (x, y, dy) from a file.
    /// @param filename The name of the file containing the data.
    /// @param column_x The column for the x values (default is 1).
    /// @param column_y The column for the y values (default is 2).
    /// @param column_dy The column for the error values (default is 3).
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &plotfile_xy_err(const std::string &filename,
                             const unsigned int column_x  = 1,
                             const unsigned int column_y  = 2,
                             const unsigned int column_dy = 3,
                             const std::string &title     = "");

    /// @brief Plots x, y pairs with error bars (x, y, dy).
    /// @tparam X The type of the x data.
    /// @tparam Y The type of the y data.
    /// @tparam E The type of the error data.
    /// @param x The x values.
    /// @param y The y values.
    /// @param dy The error values.
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    template <typename X, typename Y, typename E>
    Gnuplot &plot_xy_err(const X &x, const Y &y, const E &dy, const std::string &title = "");

    /// @brief Plots x, y, z triples of data from a file.
    /// @param filename The name of the file containing the data.
    /// @param column_x The column for the x values (default is 1).
    /// @param column_y The column for the y values (default is 2).
    /// @param column_z The column for the z values (default is 3).
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &plotfile_xyz(const std::string &filename,
                          const unsigned int column_x = 1,
                          const unsigned int column_y = 2,
                          const unsigned int column_z = 3,
                          const std::string &title    = "");

    /// @brief Plots x, y, z triples of data.
    /// @tparam X The type of the x data.
    /// @tparam Y The type of the y data.
    /// @tparam Z The type of the z data.
    /// @param x The x values.
    /// @param y The y values.
    /// @param z The z values.
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    template <typename X, typename Y, typename Z>
    Gnuplot &plot_xyz(const X &x, const Y &y, const Z &z, const std::string &title = "");

    /// @brief Plots a 3D grid of data points.
    /// @param x A vector of x-coordinates.
    /// @param y A vector of y-coordinates.
    /// @param z A 2D vector of z-values (size: x.size() × y.size()).
    /// @param title Title for the plot.
    /// @return Reference to the current Gnuplot object.
    template <typename X, typename Y, typename Z>
    Gnuplot &plot_3d_grid(const X &x, const Y &y, const Z &z, const std::string &title = "");

    /// @brief Plots a linear equation of the form y = ax + b.
    /// @param a The slope of the line.
    /// @param b The y-intercept of the line.
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &plot_slope(const double a, const double b, const std::string &title = "");

    /// @brief Plots a 2D equation of the form y = f(x).
    /// @param equation The equation to plot (e.g., "sin(x)").
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &plot_equation(const std::string &equation, const std::string &title = "");

    /// @brief Plots a 3D equation of the form z = f(x, y).
    /// @param equation The equation to plot (e.g., "x^2 + y^2").
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &plot_equation3d(const std::string &equation, const std::string &title = "");

    /// @brief Plots an image.
    /// @param ucPicBuf The image data buffer.
    /// @param iWidth The width of the image.
    /// @param iHeight The height of the image.
    /// @param title The title of the plot (default is an empty string).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &plot_image(const unsigned char *ucPicBuf,
                        const unsigned int iWidth,
                        const unsigned int iHeight,
                        const std::string &title = "");

    /// @brief Repeats the last plot or splot command.
    /// @details Useful for viewing the same plot with different settings or generating it for multiple devices (e.g., screen or file).
    /// @return A reference to the current Gnuplot object.
    Gnuplot &replot();

    /// @brief Resets the current Gnuplot session.
    /// @details The next plot will erase all previous ones.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &reset_plot();

    /// @brief Resets the Gnuplot session and restores all variables to their default values.
    /// @return A reference to the current Gnuplot object.
    Gnuplot &reset_all();

    /// @brief Deletes all temporary files created during the session.
    void remove_tmpfiles();

    /// @brief Checks if the current Gnuplot session is valid.
    /// @return `true` if the session is valid, `false` otherwise.
    bool is_valid() const;

private:
    /// @brief Initializes the Gnuplot session.
    /// Sets up necessary configurations and opens the Gnuplot pipe.
    void init();

    /// @brief Creates a unique temporary file and returns its name.
    ///
    /// This function generates a temporary file with a unique name
    /// using platform-specific methods (`_mktemp` on Windows and `mkstemp` on Unix).
    /// It ensures that the maximum allowed number of temporary files is not exceeded.
    /// The file is opened for writing, and its name is stored for cleanup.
    ///
    /// @param tmp A reference to an `std::ofstream` object where the file will be opened.
    ///
    /// @return The name of the created temporary file.
    ///
    /// @throws GnuplotException If the maximum number of temporary files is reached
    ///         or if the temporary file cannot be created or opened.
    std::string create_tmpfile(std::ofstream &tmp);

    /// @brief Checks if the Gnuplot executable path is valid.
    /// @return `true` if the Gnuplot path is found, `false` otherwise.
    static bool get_program_path();

    /// @brief Checks if a file is available for use.
    /// @param filename The name of the file to check.
    /// @return `true` if the file exists and is accessible, `false` otherwise.
    static bool file_ready(const std::string &filename);

    /// @brief Checks if a file exists and satisfies the specified mode.
    /// @param filename The name of the file to check.
    /// @param mode The access mode to check (e.g., read, write, execute). Defaults to 0 (existence only).
    /// @return `true` if the file exists and satisfies the mode, `false` otherwise.
    static bool file_exists(const std::string &filename, int mode = 0);

    /// @brief Converts a plot_style_t value to its corresponding Gnuplot string representation.
    /// @param style The plotting style as a plot_style_t enum value.
    /// @return A string representing the corresponding Gnuplot style.
    std::string plot_style_to_string(plot_style_t style);

    /// Converts a smooth_style value to its corresponding Gnuplot string.
    /// @param style The smoothing style to convert.
    /// @return A string representing the Gnuplot smoothing style.
    std::string smooth_style_to_string(smooth_style_t style);

    /// @brief pointer to the stream that can be used to write to the pipe
    FILE *gnuplot_pipe;
    /// @brief validation of gnuplot session
    bool valid;
    /// @brief true = 2d, false = 3d
    bool two_dim;
    /// @brief number of plots in session
    int nplots;

    /// The line width for plotted lines.
    double line_width;
    /// The style used for plotting data (e.g., lines, points, histograms).
    plot_style_t plot_style;
    /// The smoothing style applied to the data (e.g., csplines, bezier).
    smooth_style_t smooth_style;

    struct {
        contour_type_t type   = contour_type_t::none;    ///< Default: no contours
        contour_param_t param = contour_param_t::levels; ///< Default: levels
        std::vector<double> discrete_levels;             ///< For discrete contour levels
        double increment_start = 0.0;                    ///< Start of increment range
        double increment_step  = 0.1;                    ///< Step size for increments
        double increment_end   = 1.0;                    ///< End of increment range
        int levels             = 10;                     ///< Number of contour levels
    } contour;

    /// @brief list of created tmpfiles.
    std::vector<std::string> tmpfile_list;

    /// @brief number of all tmpfiles (number of tmpfiles restricted)
    static int m_tmpfile_num;
    /// @brief name of executed GNUPlot file
    static std::string m_gnuplot_filename;
    /// @brief gnuplot path
    static std::string m_gnuplot_path;
    /// @brief standart terminal, used by showonscreen
    static std::string m_terminal_std;
};

} // namespace gnuplotcpp

#include "gnuplot.i.hpp"