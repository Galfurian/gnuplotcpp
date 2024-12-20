/// @file gnuplot.i.hpp
/// @brief

#pragma once

namespace gnuplotcpp
{

/// @brief Maximum number of temporary files allowed.
/// @details This value is platform-dependent:
/// - Windows: 27 files (due to OS restrictions).
/// - UNIX-like systems: 64 files.
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
#define GP_MAX_TMP_FILES 27 ///< Maximum temporary files for Windows.
#else
#define GP_MAX_TMP_FILES 64 ///< Maximum temporary files for UNIX-like systems.
#endif

// Initialize the static variables
int Gnuplot::m_tmpfile_num = 0;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
// Windows-specific static variable initializations
std::string Gnuplot::m_gnuplot_filename = "pgnuplot.exe";
std::string Gnuplot::m_gnuplot_path     = "C:/program files/gnuplot/bin/";
std::string Gnuplot::m_terminal_std     = "windows";
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
// UNIX-like system static variable initializations
std::string Gnuplot::m_gnuplot_filename = "gnuplot";
std::string Gnuplot::m_gnuplot_path     = "/usr/local/bin/";
#if defined(__APPLE__)
std::string Gnuplot::m_terminal_std = "aqua";
#else
std::string Gnuplot::m_terminal_std = "x11";
#endif
#endif

/// Macro to create a temporary file using platform-specific functions
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
#define CREATE_TEMP_FILE(name) (_mktemp(name) == NULL)
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#define CREATE_TEMP_FILE(name) (mkstemp(name) == -1)
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
#define CLOSE_PIPE(pipe) _pclose(pipe)
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#define CLOSE_PIPE(pipe) pclose(pipe)
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
#define OPEN_PIPE(cmd, mode) _popen(cmd, mode)
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#define OPEN_PIPE(cmd, mode) popen(cmd, mode)
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
#define FILE_ACCESS(file, mode) _access(file, mode)
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#define FILE_ACCESS(file, mode) access(file, mode)
#endif

Gnuplot::Gnuplot(plot_style_t style) : gnuplot_pipe(NULL), valid(false), two_dim(false), nplots(0)

{
    init();
    set_style(style);
}

Gnuplot::Gnuplot(const std::vector<double> &x,
                 const std::string &title,
                 plot_style_t style,
                 const std::string &labelx,
                 const std::string &labely)
    : gnuplot_pipe(NULL), valid(false), two_dim(false), nplots(0)
{
    init();

    set_style(style);
    set_xlabel(labelx);
    set_ylabel(labely);

    plot_x(x, title);
}

Gnuplot::Gnuplot(const std::vector<double> &x,
                 const std::vector<double> &y,
                 const std::string &title,
                 plot_style_t style,
                 const std::string &labelx,
                 const std::string &labely)
    : gnuplot_pipe(NULL), valid(false), two_dim(false), nplots(0)
{
    init();

    set_style(style);
    set_xlabel(labelx);
    set_ylabel(labely);

    plot_xy(x, y, title);
}

Gnuplot::Gnuplot(const std::vector<double> &x,
                 const std::vector<double> &y,
                 const std::vector<double> &z,
                 const std::string &title,
                 plot_style_t style,
                 const std::string &labelx,
                 const std::string &labely,
                 const std::string &labelz)
    : gnuplot_pipe(NULL), valid(false), two_dim(false), nplots(0)
{
    init();

    set_style(style);
    set_xlabel(labelx);
    set_ylabel(labely);
    set_zlabel(labelz);

    plot_xyz(x, y, z, title);
}

Gnuplot::~Gnuplot()
{
    // A stream opened by popen() should be closed by pclose()
    if (CLOSE_PIPE(gnuplot_pipe) == -1) {
        std::cerr << "Problem closing communication to gnuplot" << "\n";
    }
}

Gnuplot &Gnuplot::send_cmd(const std::string &cmdstr)
{
    if (valid) {
        // int fputs ( const char * str, FILE * stream );
        // writes the string str to the stream.
        // The function begins copying from the address specified (str) until it
        // reaches the terminating null character ('\0'). This final
        // null-character is not copied to the stream.
        fprintf(gnuplot_pipe, "%s\n", cmdstr.c_str());
        // int fflush ( FILE * stream );
        // If the given stream was open for writing and the last i/o operation was
        // an output operation, any unwritten data in the output buffer is written
        // to the file.  If the argument is a null pointer, all open files are
        // flushed.  The stream remains open after this call.
        fflush(gnuplot_pipe);

        if (cmdstr.find("replot") != std::string::npos) {
            return *this;
        } else if (cmdstr.find("splot") != std::string::npos) {
            two_dim = false;
            nplots++;
        } else if (cmdstr.find("plot") != std::string::npos) {
            two_dim = true;
            nplots++;
        }
    }
    return *this;
}

template <typename X>
Gnuplot &Gnuplot::plot_x(const X &x, const std::string &title)
{ // Check if the input vector is empty
    if (x.empty()) {
        throw GnuplotException("Input vector is empty. Cannot plot data.");
    }

    // Create a temporary file for storing the data
    std::ofstream file;
    std::string name;
    try {
        name = this->create_tmpfile(file);
    } catch (const GnuplotException &e) {
        throw GnuplotException("Failed to create a temporary file: " + std::string(e.what()));
    }

    // Ensure the temporary file name is valid
    if (name.empty()) {
        throw GnuplotException("Temporary file name is empty. File creation failed.");
    }

    // Write the data to the temporary file.
    for (size_t i = 0; i < x.size(); ++i) {
        // Check if writing succeeded.
        if (!(file << x[i] << '\n')) {
            file.close();
            throw GnuplotException("Failed to write data to the temporary file: " + name);
        }
    }

    // Ensure the file buffer is flushed and file is closed.
    file.flush();
    if (file.fail()) { // Check for flush failure
        file.close();
        throw GnuplotException("Failed to flush data to the temporary file: " + name);
    }
    file.close();

    // Pass the temporary file to Gnuplot for plotting.
    try {
        this->plotfile_x(name, 1, title);
    } catch (const GnuplotException &e) {
        throw GnuplotException("Failed to plot data from the temporary file: " + std::string(e.what()));
    }

    return *this;
}

template <typename X>
Gnuplot &Gnuplot::plot_x(const std::vector<X> &x, const std::vector<std::string> &title)
{
    if (x.size() == 0) // no check
    {
        throw GnuplotException("std::vector too small");
        return *this;
    }

    int column = 1;
    std::ostringstream oss;
    //
    // command to be sent to gnuplot
    //
    if (nplots > 0 && two_dim == true)
        oss << "replot ";
    else
        oss << "plot ";

    for (unsigned int k = 0; k < x.size(); k++) {
        oss << "\'-\' using " << column;
        if (title.size() == 0 || title[k] == "") {
            oss << " notitle ";
        } else {
            oss << " title \"" << title[k] << "\" ";
        }
        if (smooth_style != smooth_style_t::none) {
            oss << "smooth " + smooth_style_to_string(smooth_style);
        } else {
            oss << "with " + plot_style_to_string(plot_style);
        }
        // Add line width if applicable.
        if (line_width > 0) {
            oss << " lw " << line_width;
        }
        if (k != x.size() - 1) {
            oss << ",";
        }
    }

    oss << "\n";
    for (unsigned int k = 0; k < x.size(); k++) {
        typename X::const_iterator it;
        it = x[k].begin();
        for (; it != x[k].end(); it++)
            oss << (*it) << std::endl;
        oss << "e" << std::endl;
    }

    //
    // Do the actual plot
    //
    this->send_cmd(oss.str()); //nplots++; two_dim = true;  already in cmd();

    return *this;
}

template <typename X, typename Y>
Gnuplot &Gnuplot::plot_xy(const X &x, const Y &y, const std::string &title)
{
    if (x.size() == 0 || y.size() == 0) {
        throw GnuplotException("std::vectors too small");
        return *this;
    }

    if (x.size() != y.size()) {
        throw GnuplotException("Length of the std::vectors differs");
        return *this;
    }

    std::ofstream file;
    std::string name = create_tmpfile(file);
    if (name == "")
        return *this;

    //
    // write the data to file
    //
    for (unsigned int i = 0; i < x.size(); i++)
        file << x[i] << " " << y[i] << std::endl;

    file.flush();
    file.close();

    plotfile_xy(name, 1, 2, title);

    return *this;
}

template <typename X, typename Y, typename E>
Gnuplot &Gnuplot::plot_xy_err(const X &x, const Y &y, const E &dy, const std::string &title)
{
    if (x.size() == 0 || y.size() == 0 || dy.size() == 0) {
        throw GnuplotException("std::vectors too small");
        return *this;
    }

    if (x.size() != y.size() || y.size() != dy.size()) {
        throw GnuplotException("Length of the std::vectors differs");
        return *this;
    }

    std::ofstream file;
    std::string name = create_tmpfile(file);
    if (name == "")
        return *this;

    //
    // write the data to file
    //
    for (unsigned int i = 0; i < x.size(); i++)
        file << x[i] << " " << y[i] << " " << dy[i] << std::endl;

    file.flush();
    file.close();

    // Do the actual plot
    plotfile_xy_err(name, 1, 2, 3, title);
    return *this;
}

template <typename X, typename Y, typename Z>
Gnuplot &Gnuplot::plot_xyz(const X &x, const Y &y, const Z &z, const std::string &title)
{
    if (x.size() == 0 || y.size() == 0 || z.size() == 0) {
        throw GnuplotException("std::vectors too small");
        return *this;
    }

    if (x.size() != y.size() || x.size() != z.size()) {
        throw GnuplotException("Length of the std::vectors differs");
        return *this;
    }

    std::ofstream file;
    std::string name = create_tmpfile(file);
    if (name == "") {
        return *this;
    }

    //
    // write the data to file
    //
    for (unsigned int i = 0; i < x.size(); i++)
        file << x[i] << " " << y[i] << " " << z[i] << std::endl;

    file.flush();
    file.close();

    plotfile_xyz(name, 1, 2, 3, title);

    return *this;
}

template <typename X, typename Y, typename Z>
Gnuplot &Gnuplot::plot_3d_grid(const X &x, const Y &y, const Z &z, const std::string &title)
{
    // Validate input sizes
    if (x.empty() || y.empty() || z.empty()) {
        std::cerr << "Error: Input vectors must not be empty.\n";
        return *this;
    }
    if (z.size() != x.size() || z[0].size() != y.size()) {
        std::cerr << "Error: Dimensions of z must match x and y sizes.\n";
        return *this;
    }

    // Create a temporary file
    std::ofstream file;
    std::string name = create_tmpfile(file);
    if (name.empty()) {
        return *this;
    }

    // Write the grid data to the file
    for (size_t i = 0; i < x.size(); ++i) {
        for (size_t j = 0; j < y.size(); ++j) {
            file << x[i] << " " << y[j] << " " << z[i][j] << std::endl;
        }
        file << "\n"; // Separate rows for Gnuplot
    }

    file.flush();
    file.close();

    // Call the next-level function
    plotfile_xyz(name, 1, 2, 3, title);

    return *this;
}

bool Gnuplot::set_gnuplot_path(const std::string &path)
{
    std::string tmp = path + "/" + Gnuplot::m_gnuplot_filename;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    if (Gnuplot::file_exists(tmp, 0)) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if (Gnuplot::file_exists(tmp, 1)) // check existence and execution permission
#endif
    {
        Gnuplot::m_gnuplot_path = path;
        return true;
    } else {
        Gnuplot::m_gnuplot_path.clear();
        return false;
    }
}

void Gnuplot::set_terminal_std(const std::string &type)
{
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if (type.find("x11") != std::string::npos && getenv("DISPLAY") == NULL) {
        throw GnuplotException("Can't find DISPLAY variable");
    }
#endif

    Gnuplot::m_terminal_std = type;
    return;
}

/// Tokenizes a string into a container based on the specified delimiters.
///
/// @tparam Container The type of the container to store tokens (e.g., std::vector<std::string>).
/// @param container Reference to the container where tokens will be stored.
/// @param in The input string to tokenize.
/// @param delimiters A string containing delimiter characters (default is whitespace).
template <typename Container>
static inline void tokenize(Container &container, const std::string &in, const std::string &delimiters = " \t\n")
{
    std::stringstream ss(in);
    std::string token;

    // Use a custom delimiter iterator if delimiters are more than whitespace.
    if (delimiters == " \t\n") {
        while (ss >> token) {
            container.push_back(token);
        }
    } else {
        std::string::size_type start = 0;
        while ((start = in.find_first_not_of(delimiters, start)) != std::string::npos) {
            auto end = in.find_first_of(delimiters, start);
            container.push_back(in.substr(start, end - start));
            start = end;
        }
    }
}

Gnuplot &Gnuplot::replot()
{
    if (nplots > 0) {
        this->send_cmd("replot");
    }
    return *this;
}

bool Gnuplot::is_valid() const
{
    return valid;
}

Gnuplot &Gnuplot::reset_plot()
{
    nplots = 0;
    return *this;
}

Gnuplot &Gnuplot::reset_all()
{
    nplots = 0;
    this->send_cmd("reset");
    this->send_cmd("clear");
    plot_style   = plot_style_t::none;
    smooth_style = smooth_style_t::none;
    showonscreen();
    return *this;
}

Gnuplot &Gnuplot::set_style(plot_style_t style)
{
    plot_style = style;
    return *this;
}

Gnuplot &Gnuplot::set_smooth(smooth_style_t style)
{
    smooth_style = style;
    return *this;
}

Gnuplot &Gnuplot::showonscreen()
{
    this->send_cmd("set output");
    this->send_cmd("set terminal " + Gnuplot::m_terminal_std);

    return *this;
}

Gnuplot &Gnuplot::savetofigure(const std::string filename, const std::string terminal)
{
    std::ostringstream oss;
    oss << "set terminal " << terminal;
    this->send_cmd(oss.str());

    oss.str(""); // Clear oss
    oss << "set output \"" << filename << "\"";
    this->send_cmd(oss.str());

    return *this;
}

Gnuplot &Gnuplot::set_legend(const std::string &position)
{
    std::ostringstream oss;
    oss << "set key " << position;

    this->send_cmd(oss.str());

    return *this;
}

Gnuplot &Gnuplot::unset_legend()
{
    this->send_cmd("unset key");
    return *this;
}

Gnuplot &Gnuplot::set_title(const std::string &title)
{
    std::string cmdstr;
    cmdstr = "set title \"";
    cmdstr += title;
    cmdstr += "\"";
    *this << cmdstr;
    return *this;
}

Gnuplot &Gnuplot::unset_title()
{
    this->set_title();
    return *this;
}

Gnuplot &Gnuplot::set_xlogscale(const double base)
{
    std::ostringstream cmdstr;

    cmdstr << "set logscale x " << base;
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_ylogscale(const double base)
{
    std::ostringstream cmdstr;

    cmdstr << "set logscale y " << base;
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_zlogscale(const double base)
{
    std::ostringstream cmdstr;

    cmdstr << "set logscale z " << base;
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::unset_xlogscale()
{
    this->send_cmd("unset logscale x");
    return *this;
}

Gnuplot &Gnuplot::unset_ylogscale()
{
    this->send_cmd("unset logscale y");
    return *this;
}

Gnuplot &Gnuplot::unset_zlogscale()
{
    this->send_cmd("unset logscale z");
    return *this;
}

Gnuplot &Gnuplot::set_pointsize(const double pointsize)
{
    std::ostringstream cmdstr;
    cmdstr << "set pointsize " << pointsize;
    this->send_cmd(cmdstr.str());
    return *this;
}

Gnuplot &Gnuplot::set_line_width(double width)
{
    if (width > 0) {
        line_width = width;
    }
    return *this;
}

/// turns grid on/off
Gnuplot &Gnuplot::set_grid()
{
    this->send_cmd("set grid");
    return *this;
};

/// grid is not set by default
Gnuplot &Gnuplot::unset_grid()
{
    this->send_cmd("unset grid");
    return *this;
};

Gnuplot &Gnuplot::set_multiplot()
{
    this->send_cmd("set multiplot");
    return *this;
};

Gnuplot &Gnuplot::unset_multiplot()
{
    this->send_cmd("unset multiplot");
    return *this;
};

Gnuplot &Gnuplot::set_samples(const int samples)
{
    std::ostringstream cmdstr;
    cmdstr << "set samples " << samples;
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_isosamples(const int isolines)
{
    std::ostringstream cmdstr;
    cmdstr << "set isosamples " << isolines;
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_contour_type(contour_type_t type)
{
    contour.type = type;
    return *this;
}

Gnuplot &Gnuplot::set_contour_param(contour_param_t param)
{
    contour.param = param;
    return *this;
}

Gnuplot &Gnuplot::set_contour_levels(int levels)
{
    if (levels > 0) {
        contour.levels = levels;
    }
    return *this;
}

Gnuplot &Gnuplot::set_contour_increment(double start, double step, double end)
{
    contour.increment_start = start;
    contour.increment_step  = step;
    contour.increment_end   = end;
    return *this;
}

Gnuplot &Gnuplot::set_contour_discrete_levels(const std::vector<double> &levels)
{
    contour.discrete_levels = levels;
    return *this;
}

Gnuplot &Gnuplot::set_hidden3d()
{
    this->send_cmd("set hidden3d");
    return *this;
};

Gnuplot &Gnuplot::unset_hidden3d()
{
    this->send_cmd("unset hidden3d");
    return *this;
};

Gnuplot &Gnuplot::unset_contour()
{
    this->send_cmd("unset contour");
    return *this;
};

Gnuplot &Gnuplot::set_surface()
{
    this->send_cmd("set surface");
    return *this;
};

Gnuplot &Gnuplot::unset_surface()
{
    this->send_cmd("unset surface");
    return *this;
}

Gnuplot &Gnuplot::set_xautoscale()
{
    this->send_cmd("set xrange restore");
    this->send_cmd("set autoscale x");
    return *this;
};

Gnuplot &Gnuplot::set_yautoscale()
{
    this->send_cmd("set yrange restore");
    this->send_cmd("set autoscale y");
    return *this;
};

Gnuplot &Gnuplot::set_zautoscale()
{
    this->send_cmd("set zrange restore");
    this->send_cmd("set autoscale z");
    return *this;
};

Gnuplot &Gnuplot::set_xlabel(const std::string &label)
{
    std::ostringstream cmdstr;

    cmdstr << "set xlabel \"" << label << "\"";
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_ylabel(const std::string &label)
{
    std::ostringstream cmdstr;

    cmdstr << "set ylabel \"" << label << "\"";
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_zlabel(const std::string &label)
{
    std::ostringstream cmdstr;

    cmdstr << "set zlabel \"" << label << "\"";
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_xrange(const double iFrom, const double iTo)
{
    std::ostringstream cmdstr;

    cmdstr << "set xrange[" << iFrom << ":" << iTo << "]";
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_yrange(const double iFrom, const double iTo)
{
    std::ostringstream cmdstr;

    cmdstr << "set yrange[" << iFrom << ":" << iTo << "]";
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_zrange(const double iFrom, const double iTo)
{
    std::ostringstream cmdstr;

    cmdstr << "set zrange[" << iFrom << ":" << iTo << "]";
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::set_cbrange(const double iFrom, const double iTo)
{
    std::ostringstream cmdstr;

    cmdstr << "set cbrange[" << iFrom << ":" << iTo << "]";
    this->send_cmd(cmdstr.str());

    return *this;
}

Gnuplot &Gnuplot::plot_slope(const double a, const double b, const std::string &title)
{
    std::ostringstream oss;
    //
    // command to be sent to gnuplot
    //
    if (nplots > 0 && two_dim == true)
        oss << "replot ";
    else
        oss << "plot ";

    oss << a << " * x + " << b << " title \"";

    if (title == "")
        oss << "f(x) = " << a << " * x + " << b;
    else
        oss << title;

    oss << "\" with " << plot_style_to_string(plot_style);

    // Add line width if applicable.
    if (line_width > 0) {
        oss << " lw " << line_width;
    }

    //
    // Do the actual plot
    //
    this->send_cmd(oss.str());

    return *this;
}

Gnuplot &Gnuplot::plot_equation(const std::string &equation, const std::string &title)
{
    std::ostringstream oss;
    //
    // command to be sent to gnuplot
    //
    if (nplots > 0 && two_dim == true)
        oss << "replot ";
    else
        oss << "plot ";

    oss << equation;

    if (title == "")
        oss << " notitle";
    else
        oss << " title \"" << title << "\"";

    oss << " with " << plot_style_to_string(plot_style);

    // Add line width if applicable.
    if (line_width > 0) {
        oss << " lw " << line_width;
    }

    //
    // Do the actual plot
    //
    this->send_cmd(oss.str());

    return *this;
}

Gnuplot &Gnuplot::plot_equation3d(const std::string &equation, const std::string &title)
{
    std::ostringstream oss;
    //
    // command to be sent to gnuplot
    //
    if (nplots > 0 && two_dim == false)
        oss << "replot ";
    else
        oss << "splot ";

    oss << equation << " title \"";

    if (title == "")
        oss << "f(x,y) = " << equation;
    else
        oss << title;

    oss << "\" with " << plot_style_to_string(plot_style);

    // Add line width if applicable.
    if (line_width > 0) {
        oss << " lw " << line_width;
    }

    //
    // Do the actual plot
    //
    this->send_cmd(oss.str());

    return *this;
}

Gnuplot &Gnuplot::plotfile_x(const std::string &filename, const unsigned int column, const std::string &title)
{
    // Check if the file is available for reading
    if (this->file_ready(filename)) {
        std::ostringstream oss;
        // Determine whether to use 'plot' or 'replot' based on the current plot state.
        oss << ((nplots > 0 && two_dim) ? "replot" : "plot");
        // Specify the file and columns for the Gnuplot command.
        oss << " \"" << filename << "\" using " << column;
        // Add a title or specify 'notitle' if no title is provided.
        oss << (title.empty() ? " notitle " : " title \"" + title + "\"");
        // Specify the plot style or smoothing option.
        if (smooth_style != smooth_style_t::none) {
            oss << " smooth " + smooth_style_to_string(smooth_style);
        } else {
            oss << " with " + plot_style_to_string(plot_style);
        }
        // Add line width if applicable.
        if (line_width > 0) {
            oss << " lw " << line_width;
        }
        // Send the constructed command to Gnuplot for execution
        this->send_cmd(oss.str());
    }
    return *this;
}

Gnuplot &Gnuplot::plotfile_xy(const std::string &filename,
                              const unsigned int column_x,
                              const unsigned int column_y,
                              const std::string &title)
{
    // Check if the file is available for reading
    if (this->file_ready(filename)) {
        std::ostringstream oss;
        // Determine whether to use 'plot' or 'replot' based on the current plot state
        oss << ((nplots > 0 && two_dim) ? "replot" : "plot");
        // Specify the file and columns for the Gnuplot command
        oss << " \"" << filename << "\" using " << column_x << ":" << column_y;
        // Add a title or specify 'notitle' if no title is provided
        oss << (title.empty() ? " notitle " : " title \"" + title + "\"");
        // Specify the plot style or smoothing option
        if (smooth_style != smooth_style_t::none) {
            oss << " smooth " + smooth_style_to_string(smooth_style);
        } else {
            oss << " with " + plot_style_to_string(plot_style);
        }
        // Add line width if applicable.
        if (line_width > 0) {
            oss << " lw " << line_width;
        }
        // Send the constructed command to Gnuplot for execution
        this->send_cmd(oss.str());
    }
    return *this;
}

Gnuplot &Gnuplot::plotfile_xy_err(const std::string &filename,
                                  const unsigned int column_x,
                                  const unsigned int column_y,
                                  const unsigned int column_dy,
                                  const std::string &title)
{
    // Check if the file is available for reading
    if (this->file_ready(filename)) {
        std::ostringstream oss;
        // Determine whether to use 'plot' or 'replot' based on the current plot state
        oss << ((nplots > 0 && two_dim) ? "replot " : "plot ");
        // Specify the file and columns for the Gnuplot command
        oss << "\"" << filename << "\" using " << column_x << ":" << column_y << ":" << column_dy << " with errorbars ";
        // Add a title or specify 'notitle' if no title is provided
        oss << (title.empty() ? " notitle " : " title \"" + title + "\" ");
        // Send the constructed command to Gnuplot for execution
        this->send_cmd(oss.str());
    }
    return *this;
}

Gnuplot &Gnuplot::plotfile_xyz(const std::string &filename,
                               const unsigned int column_x,
                               const unsigned int column_y,
                               const unsigned int column_z,
                               const std::string &title)
{
    // Check if the file is available for reading
    if (this->file_ready(filename)) {
        std::ostringstream oss;
        // Determine whether to use 'splot' or 'replot' based on the current plot state
        oss << ((nplots > 0 && !two_dim) ? "replot" : "splot");
        // Specify the file and columns for the Gnuplot command
        oss << " \"" << filename << "\" using " << column_x << ":" << column_y << ":" << column_z;
        // Add a title or specify 'notitle' if no title is provided
        oss << (title.empty() ? " notitle" : " title \"" + title + "\"");
        // Add the style.
        oss << " with " + plot_style_to_string(plot_style);
        // Add line width if applicable.
        if (line_width > 0) {
            oss << " lw " << line_width;
        }
        // Send the constructed command to Gnuplot for execution
        this->send_cmd(oss.str());
    }
    return *this;
}

Gnuplot &Gnuplot::plot_image(const unsigned char *ucPicBuf,
                             const unsigned int iWidth,
                             const unsigned int iHeight,
                             const std::string &title)
{
    // Create a temporary file to store image data
    std::ofstream file;
    std::string filename = create_tmpfile(file);
    if (filename.empty()) {
        std::cerr << "Error: Failed to create a temporary file for image plotting." << std::endl;
        return *this; // Early return on failure
    }

    // Write the image data (width, height, pixel value) to the temporary file
    int iIndex         = 0;
    bool write_success = true;
    for (unsigned int iRow = 0; iRow < iHeight && write_success; ++iRow) {
        for (unsigned int iColumn = 0; iColumn < iWidth; ++iColumn) {
            if (!(file << iColumn << " " << iRow << " " << static_cast<float>(ucPicBuf[iIndex++]) << std::endl)) {
                std::cerr << "Error: Failed to write image data to temporary file: " << filename << std::endl;
                write_success = false;
                break;
            }
        }
    }

    // Ensure all data is written to the file and the file is closed properly
    file.flush();
    if (file.fail() || !write_success) {
        file.close();
        std::cerr << "Error: Failed to flush data to temporary file: " << filename << std::endl;
        return *this; // Early return on failure
    }
    file.close();

    // Check if the file is available for reading
    if (this->file_ready(filename)) {
        // Construct the Gnuplot command for plotting the image
        std::ostringstream oss;
        // Determine whether to use 'plot' or 'replot' based on the current plot state
        oss << ((nplots > 0 && two_dim) ? "replot " : "plot ");
        // Specify the file and plotting options
        oss << "\"" << filename << "\" with image";
        if (!title.empty()) {
            oss << " title \"" << title << "\"";
        }
        // Send the constructed command to Gnuplot for execution
        this->send_cmd(oss.str());
    }
    return *this;
}

void Gnuplot::init()
{
    // char * getenv ( const char * name );  get value of environment variable
    // Retrieves a C string containing the value of the environment variable
    // whose name is specified as argument.  If the requested variable is not
    // part of the environment list, the function returns a NULL pointer.
#if (defined(unix) || defined(__unix) || defined(__unix__)) && !defined(__APPLE__)
    if (getenv("DISPLAY") == NULL) {
        valid = false;
        throw GnuplotException("Can't find DISPLAY variable");
    }
#endif

    // if gnuplot not available
    if (!Gnuplot::get_program_path()) {
        valid = false;
        throw GnuplotException("Can't find gnuplot");
    }

    //
    // open pipe
    //
    std::string tmp = Gnuplot::m_gnuplot_path + "/" + Gnuplot::m_gnuplot_filename;

    // FILE *popen(const char *command, const char *mode);
    // The popen() function shall execute the command specified by the string
    // command, create a pipe between the calling program and the executed
    // command, and return a pointer to a stream that can be used to either read
    // from or write to the pipe.
    gnuplot_pipe = OPEN_PIPE(tmp.c_str(), "w");

    // popen() shall return a pointer to an open stream that can be used to read
    // or write to the pipe.  Otherwise, it shall return a null pointer and may
    // set errno to indicate the error.
    if (!gnuplot_pipe) {
        valid = false;
        throw GnuplotException("Couldn't open connection to gnuplot");
    }

    nplots       = 0;
    valid        = true;
    smooth_style = smooth_style_t::none;
    plot_style   = plot_style_t::none;

    //set terminal type
    showonscreen();

    return;
}

bool Gnuplot::get_program_path()
{
    //
    // first look in m_gnuplot_path for Gnuplot
    //
    std::string tmp = Gnuplot::m_gnuplot_path + "/" + Gnuplot::m_gnuplot_filename;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    if (Gnuplot::file_exists(tmp, 0)) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if (Gnuplot::file_exists(tmp, 1)) // check existence and execution permission
#endif
    {
        return true;
    }

    //
    // second look in PATH for Gnuplot
    //
    char *path;
    // Retrieves a C string containing the value of environment variable PATH
    path = getenv("PATH");

    if (path == NULL) {
        throw GnuplotException("Path is not set");
        return false;
    } else {
        std::list<std::string> ls;

        //split path (one long string) into list ls of strings
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
        tokenize(ls, path, ";");
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
        tokenize(ls, path, ":");
#endif

        // scan list for Gnuplot program files
        for (std::list<std::string>::const_iterator i = ls.begin(); i != ls.end(); ++i) {
            tmp = (*i) + "/" + Gnuplot::m_gnuplot_filename;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
            if (Gnuplot::file_exists(tmp, 0)) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
            if (Gnuplot::file_exists(tmp, 1)) // check existence and execution permission
#endif
            {
                Gnuplot::m_gnuplot_path = *i; // set m_gnuplot_path
                return true;
            }
        }

        tmp = "Can't find gnuplot neither in PATH nor in \"" + Gnuplot::m_gnuplot_path + "\"";
        throw GnuplotException(tmp);
    }
}

bool Gnuplot::file_exists(const std::string &filename, int mode)
{
    if (mode < 0 || mode > 7) {
        throw std::runtime_error("In function \"Gnuplot::file_exists\": mode\
                has to be an integer between 0 and 7");
        return false;
    }

    // int _access(const char *path, int mode);
    //  returns 0 if the file has the given mode,
    //  it returns -1 if the named file does not exist or is not accessible in
    //  the given mode
    // mode = 0 (F_OK) (default): checks file for existence only
    // mode = 1 (X_OK): execution permission
    // mode = 2 (W_OK): write permission
    // mode = 4 (R_OK): read permission
    // mode = 6       : read and write permission
    // mode = 7       : read, write and execution permission
    return (FILE_ACCESS(filename.c_str(), mode) == 0);
}

bool Gnuplot::file_ready(const std::string &filename)
{
    // Check if the file exists.
    if (Gnuplot::file_exists(filename, 0)) {
        // Check if the file has read permissions.
        if (Gnuplot::file_exists(filename, 4)) {
            return true;
        }
        std::cerr << "No read permission for file \"" << filename << "\".";
        return false;
    }
    // File does not exist.
    std::cerr << "File \"" << filename << "\" does not exist.";
    return true;
}

std::string Gnuplot::create_tmpfile(std::ofstream &tmp)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    char filename[] = "gnuplotiXXXXXX"; //tmp file in working directory
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    char filename[] = "/tmp/gnuplotiXXXXXX"; // tmp file in /tmp
#endif

    // Check if the maximum number of temporary files has been reached.
    if (Gnuplot::m_tmpfile_num == GP_MAX_TMP_FILES - 1) {
        std::ostringstream except;
        except << "Maximum number of temporary files reached (" << GP_MAX_TMP_FILES << "): cannot open more files.\n";
        throw GnuplotException(except.str());
    }

    // Generate a unique temporary filename and open it.
    if (CREATE_TEMP_FILE(filename)) {
        std::ostringstream except;
        except << "Cannot create temporary file \"" << filename << "\".";
        throw GnuplotException(except.str());
    }

    // Open the temporary file for writing.
    tmp.open(filename);
    if (tmp.bad()) {
        std::ostringstream except;
        except << "Cannot open temporary file \"" << filename << "\" for writing.";
        throw GnuplotException(except.str());
    }

    // Store the temporary file filename for cleanup and increment the counter.
    tmpfile_list.push_back(filename);
    Gnuplot::m_tmpfile_num++;

    return filename;
}

Gnuplot &Gnuplot::apply_contour_settings()
{
    // Set contour type.
    switch (contour.type) {
    case contour_type_t::base:
        this->send_cmd("set contour base");
        break;
    case contour_type_t::surface:
        this->send_cmd("set contour surface");
        break;
    case contour_type_t::both:
        this->send_cmd("set contour both");
        break;
    case contour_type_t::none:
        this->send_cmd("unset contour");
        break;
    }
    // Return early if no contour settings are specified.
    if (contour.type == contour_type_t::none) {
        return *this;
    }
    // Set contour parameters.
    switch (contour.param) {
    case contour_param_t::levels:
        this->send_cmd("set cntrparam levels " + std::to_string(contour.levels));
        break;
    case contour_param_t::increment: {
        std::ostringstream oss;
        oss << "set cntrparam increment " << contour.increment_start << "," << contour.increment_step << ","
            << contour.increment_end;
        this->send_cmd(oss.str());
        break;
    }
    case contour_param_t::discrete: {
        std::ostringstream oss;
        oss << "set cntrparam level discrete";
        for (std::size_t i = 0; i < contour.discrete_levels.size(); ++i) {
            oss << " " << contour.discrete_levels[i];
            if (i < contour.discrete_levels.size() - 1) {
                oss << ",";
            }
        }
        this->send_cmd(oss.str());
        break;
    }
    }
    return *this;
}

std::string Gnuplot::plot_style_to_string(plot_style_t style)
{
    switch (style) {
    case plot_style_t::lines:
        return "lines";
    case plot_style_t::points:
        return "points";
    case plot_style_t::lines_points:
        return "linespoints";
    case plot_style_t::impulses:
        return "impulses";
    case plot_style_t::dots:
        return "dots";
    case plot_style_t::steps:
        return "steps";
    case plot_style_t::fsteps:
        return "fsteps";
    case plot_style_t::histeps:
        return "histeps";
    case plot_style_t::boxes:
        return "boxes";
    case plot_style_t::filled_curves:
        return "filledcurves";
    case plot_style_t::histograms:
        return "histograms";
    default:
        return "points"; // Default fallback
    }
}

std::string Gnuplot::smooth_style_to_string(smooth_style_t style)
{
    switch (style) {
    case smooth_style_t::unique:
        return "unique";
    case smooth_style_t::frequency:
        return "frequency";
    case smooth_style_t::csplines:
        return "csplines";
    case smooth_style_t::acsplines:
        return "acsplines";
    case smooth_style_t::bezier:
        return "bezier";
    case smooth_style_t::sbezier:
        return "sbezier";
    default:
        return ""; // Default: No smoothing
    }
}

void Gnuplot::remove_tmpfiles()
{
    if ((tmpfile_list).size() > 0) {
        for (unsigned int i = 0; i < tmpfile_list.size(); i++) {
            if (remove(tmpfile_list[i].c_str()) != 0) {
                std::ostringstream except;
                except << "Cannot remove temporary file \"" << tmpfile_list[i] << "\"";
                throw GnuplotException(except.str());
            }
        }

        Gnuplot::m_tmpfile_num -= static_cast<int>(tmpfile_list.size());
    }
}

} // namespace gnuplotcpp
