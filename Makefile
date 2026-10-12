NAME        = n-puzzle
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++17 -O2
INCLUDES    = -I include

SRCDIR      = src
OBJDIR      = obj

SRCS        = $(wildcard $(SRCDIR)/*.cpp)
OBJS        = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
