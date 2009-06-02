
#include "pam_url.h"

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	pam_url_opts opts;
	int ret=0;
	char *newp1 = NULL, *newp2 = NULL;
	char *tmp = NULL;

	if( PAM_PRELIM_CHECK == flags )
	{ // TODO: Connection checks?
		return PAM_SUCCESS;
	}

	if ( PAM_SUCCESS != pam_get_item(pamh, PAM_USER, &opts.user) )
	{
		ret++;
		debug(pamh, "Could not get user item from pam.");
	}

	if( PAM_SUCCESS != parse_opts(&opts, argc, argv, PAM_SM_PASSWORD) )
	{
		ret++;
		debug(pamh, "Could not parse module options.");
	}

	pam_get_item(pamh, PAM_OLDAUTHTOK, &opts.passwd);
	if( NULL == opts.passwd )
	{
		pam_prompt(pamh, PAM_PROMPT_ECHO_OFF, (char**)&opts.passwd, "%s", "   CURRENT password: ");
	}

	pam_get_item(pamh, PAM_AUTHTOK, (const void**)&newp1);
	if( NULL == newp1 )
	{
		pam_prompt(pamh, PAM_PROMPT_ECHO_OFF, &newp1, "%s"," Enter NEW password: ");
		pam_prompt(pamh, PAM_PROMPT_ECHO_OFF, &newp2, "%s","Retype NEW password: ");
		if( 0 != strcmp(newp1,newp2) )
		{
			ret++;
			return PAM_AUTHTOK_ERR;
		}
	}

	opts.extrafield = realloc(opts.extrafield, strlen(opts.extrafield) +
												strlen("&newpass=") +
												strlen(newp1) + 1);
	tmp = calloc(1, strlen(opts.extrafield) );
	sprintf(tmp, "%s", opts.extrafield );
	sprintf(opts.extrafield, "&newpass=%s%s", newp1, tmp);
	free(tmp);

	if( PAM_SUCCESS != fetch_url(opts) )
	{
		ret++;
		debug(pamh, "Could not fetch URL.");
	}

	if( PAM_SUCCESS != check_psk(opts) )
	{
		ret++;
		debug(pamh, "Pre Shared Key differs from ours.");
	}

	cleanup(&opts);

	if( 0 == ret )
	{
		return PAM_SUCCESS;
	}
	else
	{
		debug(pamh, "Password change failed.");
		return PAM_AUTHTOK_ERR;
	}
}
