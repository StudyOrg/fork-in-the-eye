#pragma once

int release_free_pipes(void * data) {
  Router *rt = (Router*)data;

	for(int i = 0; i < rt->procnum; i++) {
		for(int j = 0; j < rt->procnum; j++) {
			if(i == j)
				continue;

			if(i == rt->recent_pid) {
				close(rt->routes[rt->recent_pid][j].filedes[1]);
				continue;
			}

			if(i != rt->recent_pid && j != rt->recent_pid) {
				close(rt->routes[i][j].filedes[1]);
				close(rt->routes[i][j].filedes[0]);
				continue;
			}

			close(rt->routes[i][j].filedes[0]);
		}
	}

	return 0;
}
